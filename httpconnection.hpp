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

	void respond(HttpResponsePtr &r) {
		auto that = shared_from_this();
		auto write_handler =
			[that](boost::system::error_code e, size_t) {
				if(e) {
					std::cerr << "Write Error: " << e.message() << std::endl;
					return;
				}
				// TODO: if connection-type wasn't keepalive:
				//boost::system::error_code ec;
				//socket_.shutdown(tcp::socket::shutdown_both, ec);
		};

		// TODO: in async_write, we need to make sure the buffer exists
		// at least until the write_handler is done executing. Currently, we
		// only guarantee the buffer exists until respond() is done, but the
		// write is async so this is not enough.
		std::string headers = r->toHeaders();
		socket_->async_write(headers, write_handler);

		std::string body = r->body();
		socket_->async_write(body, write_handler);
	}

	BaseSocketPtr socket_;
	RequestHandler delegate_;
	boost::array<char, 8192> buffer_;
	std::string work_in_progress_;
};

}}
