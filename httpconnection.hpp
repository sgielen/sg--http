#pragma once

#include "httpmessages.hpp"
#include "http_global.hpp"
#include "httpexception.hpp"
#include "socket.hpp"
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace skynet {

class HttpConnection :
	public boost::enable_shared_from_this<HttpConnection>
{
	typedef boost::asio::ip::tcp tcp;
public:
	HttpConnection(BaseSocketPtr socket,
		HttpServerDelegatePtr &delegate)
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
					std::cerr << "Read Error: " << e << std::endl;
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
			HttpResponsePtr response;
			try {
				response = delegate_->handleRequest(request);
			} catch(HttpException &e) {
				response = HttpResponsePtr(new HttpResponse(e.code()));
				response->setBody(e.body(), "text/plain");
			} catch(std::exception &e) {
				response = HttpResponsePtr(new HttpResponse(500));
				response->setBody("Internal server error: " + std::string(e.what()), "text/plain");
			} catch(...) {
				response = HttpResponsePtr(new HttpResponse(500));
				response->setBody("Internal server error", "text/plain");
			}
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
		std::vector<boost::asio::const_buffer> sendbuffers;

		const std::string sendmsg = r->toString();
		sendbuffers.push_back(boost::asio::buffer(sendmsg));

		auto that = shared_from_this();
		socket_->async_write(sendbuffers,
			[that](boost::system::error_code e, size_t) {
				if(e) {
					std::cerr << "Write Error: " << e << std::endl;
					return;
				}
				// TODO: if connection-type wasn't keepalive:
				//boost::system::error_code ec;
				//socket_.shutdown(tcp::socket::shutdown_both, ec);
			}
		);
	}

	BaseSocketPtr socket_;
	HttpServerDelegatePtr &delegate_;
	boost::array<char, 8192> buffer_;
	std::string work_in_progress_;
};

}
