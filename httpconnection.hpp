#pragma once

#include "httpmessages.hpp"
#include "http_global.hpp"
#include "httpexception.hpp"
#include "socket.hpp"
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace sg { namespace http {

class HttpConnection :
	public boost::enable_shared_from_this<HttpConnection>
{
	typedef boost::asio::ip::tcp tcp;
public:
	HttpConnection(BaseSocketPtr socket, RequestHandler delegate)
	: socket_(socket)
	, delegate_(delegate)
	{}

	void start() {
		auto that = shared_from_this();
		socket_->async_start(
			[that]() { that->read_some(); }
		);
	}

	void read_some() {
		auto that = shared_from_this();
		socket_->async_read_some(boost::asio::buffer(buffer_),
			[that](boost::system::error_code e, size_t bytes_transferred) {
				if(e) {
					//std::cerr << "Read Error: " << e.message() << std::endl;
					return;
				}
				that->work_in_progress_.append(that->buffer_.data(), bytes_transferred);
				that->tryHttpRequest();
			}
		);
	}

private:
	void tryHttpRequest() {
		try {
			HttpRequestPtr request(new HttpRequest(work_in_progress_));
			// TODO: buffer_ may contain multiple requests at the same time
			// HttpRequest() should take one off of it and return the length read
			work_in_progress_.clear();
			HttpResponsePtr response = request_exception_wrapper(request, delegate_);
			respond(response);
			read_some();
		} catch(HttpMessage::IncompleteHttpMessageException &) {
			read_some();
		} catch(HttpRequest::InvalidHttpMessageException &e) {
			std::cerr << "Parse Error: " << e.what() << std::endl;
			// TODO: send back Bad Request and close the connection
		}
	}

	struct ChunkedTransferer {
		std::shared_ptr<std::string> buffer;
		HttpResponsePtr response;
		BaseSocketPtr socket;

		void operator()(boost::system::error_code ec, size_t) {
			if(ec) {
				std::cerr << "Write Error: " << ec.message() << std::endl;
				socket->close();
				return;
			}

			try {
				buffer = std::make_shared<std::string>(response->readChunk());
				socket->async_write(*buffer.get(), *this);
			} catch(HttpMessage::NoChunksLeftException&) {
				// TODO: use chunked transfer-encoding and send "end of chunks" signal
				socket->close();
			}
		}
	};

	void respond(HttpResponsePtr r) {
		bool must_close = r->headers.find("Content-Length") == r->headers.end();
		std::shared_ptr<std::string> buffer = std::make_shared<std::string>(r->toHeaders());
		BaseSocketPtr socket = socket_;
		socket->async_write(*buffer.get(), [buffer, r, socket, must_close](boost::system::error_code e, size_t) mutable {
			if(e) {
				std::cerr << "Write Error: " << e.message() << std::endl;
				socket->close();
				return;
			}

			if(r->isChunked()) {
				ChunkedTransferer transfer;
				transfer.buffer = buffer;
				transfer.response = r;
				transfer.socket = socket;
				boost::system::error_code ec;
				transfer(ec, 0);
			} else if(!r->body().empty()) {
				buffer.reset(new std::string(r->body()));
				socket->async_write(*buffer.get(), [buffer, socket, must_close](boost::system::error_code ec, size_t) mutable {
					buffer.reset(); // mention buffer so the pointer gets copied into the lambda

					if(ec) {
						std::cerr << "Write Error: " << ec.message() << std::endl;
						socket->close();
						return;
					}

					if(must_close) {
						socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
						socket->close();
					}
				});
			} else if(must_close) {
				socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, e);
				socket->close();
			}
		});
	}

	BaseSocketPtr socket_;
	RequestHandler delegate_;
	boost::array<char, 8192> buffer_;
	std::string work_in_progress_;
};

}}
