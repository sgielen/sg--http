#pragma once

// Most of this code was copied from the Boost HTTP server example, and as
// such is licensed under the Boost Software License, version 1.0.

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "http_global.hpp"
#include "httpconnection.hpp"

namespace skynet {

class HttpServer {
public:
	HttpServer(std::string address, std::string port,
		size_t thread_pool_size, HttpServerDelegatePtr delegate)
	: io_service_()
	, acceptor_(io_service_)
	, thread_pool_size_(thread_pool_size)
	, delegate_(delegate) {
		tcp::resolver resolver(io_service_);
		tcp::resolver::query query(address, port);
		tcp::endpoint endpoint = *resolver.resolve(query);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();

		start_accept();
	}

	void run() {
		std::vector<boost::shared_ptr<boost::thread>> threads;
		for(size_t i = 0; i < thread_pool_size_; ++i) {
			boost::shared_ptr<boost::thread> thread(
				new boost::thread(boost::bind(
					&boost::asio::io_service::run,
					&io_service_)));
			threads.push_back(thread);
		}

		for(auto it = threads.begin(); it != threads.end(); ++it) {
			(*it)->join();
		}
	}

	void stop() {
		io_service_.stop();
	}

private:
	typedef boost::asio::ip::tcp tcp;
	void start_accept() {
		BaseSocketPtr sock(new Socket(io_service_));
		sock->async_accept(acceptor_,
			[this,sock](boost::system::error_code e) {
				if(e) {
					std::cerr << "Error accepting: " << e << std::endl;
				} else {
					HttpConnectionPtr new_connection(new HttpConnection(sock,
						delegate_));
					new_connection->start();
				}
				start_accept();
			}
		);
	}

	boost::asio::io_service io_service_;
	tcp::acceptor acceptor_;
	size_t thread_pool_size_;
	HttpServerDelegatePtr delegate_;
};

}
