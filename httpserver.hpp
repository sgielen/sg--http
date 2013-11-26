#pragma once

// Most of this code was copied from the Boost HTTP server example, and as
// such is licensed under the Boost Software License, version 1.0.

#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <memory>
#include "http_global.hpp"
#include "httpconnection.hpp"

namespace sg { namespace http {

typedef std::unique_ptr<boost::asio::ssl::context> SslContext;

class HttpServer {
public:
	HttpServer(std::string address, std::string port,
		size_t thread_pool_size, SslContext context,
		HttpServerDelegatePtr delegate)
	: io_service_()
	, acceptor_(io_service_)
	, thread_pool_size_(thread_pool_size)
	, delegate_(delegate)
	, ssl_context_(std::move(context))
	, ssl_(true) {
		init_acceptor(address, port);
		start_accept();
	}

	HttpServer(std::string address, std::string port,
		size_t thread_pool_size, HttpServerDelegatePtr delegate)
	: io_service_()
	, acceptor_(io_service_)
	, thread_pool_size_(thread_pool_size)
	, delegate_(delegate)
	, ssl_(false) {
		init_acceptor(address, port);
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

	void init_acceptor(std::string address, std::string port) {
		if(ssl_ && (port == "http" || port == "80")) {
			std::cerr << "Warning: SSL enabled, but listening on port 80, this is not recommended." << std::endl;
		}
		if(!ssl_ && (port == "https" || port == "443")) {
			std::cerr << "Warning: SSL disabled, but listening on port 443, this is not recommended." << std::endl;
		}

		tcp::resolver resolver(io_service_);
		tcp::resolver::query query(address, port);
		tcp::endpoint endpoint = *resolver.resolve(query);
		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(tcp::acceptor::reuse_address(true));
		acceptor_.bind(endpoint);
		acceptor_.listen();
	}

	void start_accept() {
		BaseSocketPtr sock;
		if(ssl_) {
			sock.reset(new SslSocket(io_service_, *ssl_context_));
		} else {
			sock.reset(new Socket(io_service_));
		}
		sock->async_accept(acceptor_,
			[this,sock](boost::system::error_code e) {
				if(e) {
					std::cerr << "Error accepting: " << e.message() << std::endl;
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
	SslContext ssl_context_;
	bool ssl_;
};

}}
