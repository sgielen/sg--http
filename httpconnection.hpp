#pragma once

#include "httpmessages.hpp"
#include "http_global.hpp"
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace skynet {

class HttpConnection :
	public boost::enable_shared_from_this<HttpConnection>
{
	typedef boost::asio::ip::tcp tcp;
public:
	HttpConnection(boost::asio::io_service &io_service,
		HttpServerDelegatePtr &delegate)
	: strand_(io_service)
	, socket_(io_service)
	, delegate_(delegate)
	{}

	tcp::socket &socket() {
		return socket_;
	}

	void start() {
		read_some();
	}

	void read_some() {
		socket_.async_read_some(boost::asio::buffer(buffer_),
		    strand_.wrap(
		        boost::bind(&HttpConnection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)));
	}

private:
	void handle_read(const boost::system::error_code &e,
	    size_t bytes_transferred)
	{
		if(e) {
			std::cerr << "Read Error: " << e << std::endl;
			return;
		}

		work_in_progress_.append(buffer_.data(), bytes_transferred);

		try {
			HttpRequestPtr request(new HttpRequest(work_in_progress_));
			// TODO: buffer_ may contain multiple requests at the same time
			// HttpRequest() should take one off of it and return the length read
			work_in_progress_.clear();
			HttpResponsePtr response = delegate_->handleRequest(request);
			respond(response);
			read_some();
		} catch(HttpMessage::IncompleteHttpMessageException &) {
			read_some();
		} catch(HttpRequest::InvalidHttpMessageException &e) {
			std::cerr << "Parse Error: " << e.what() << std::endl;
			// TODO: send back Bad Request and close the connection
		}
	}

	void handle_write(const boost::system::error_code &e) {
		if(e) {
			std::cerr << "Write Error: " << e << std::endl;
			return;
		}
		// TODO: if connection-type wasn't keepalive:
		//boost::system::error_code ec;
		//socket_.shutdown(tcp::socket::shutdown_both, ec);
	}

	void respond(HttpResponsePtr &r) {
		std::vector<boost::asio::const_buffer> sendbuffers;

		const std::string sendmsg = r->toString();
		sendbuffers.push_back(boost::asio::buffer(sendmsg));

		boost::asio::async_write(socket_, sendbuffers,
			strand_.wrap(
				boost::bind(&HttpConnection::handle_write, shared_from_this(),
				boost::asio::placeholders::error)));
	}

	boost::asio::io_service::strand strand_;
	tcp::socket socket_;
	HttpServerDelegatePtr &delegate_;
	boost::array<char, 8192> buffer_;
	std::string work_in_progress_;
};

}
