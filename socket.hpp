#pragma once

#include <iostream>
#include <boost/asio.hpp>
#ifdef SG_HTTP_SSL
#include <boost/asio/ssl.hpp>
#endif

namespace sg { namespace http {

struct BaseSocket
{
	typedef boost::asio::ip::tcp tcp;

	virtual ~BaseSocket() {}

	virtual void async_accept(tcp::acceptor&, std::function<void(boost::system::error_code)>) = 0;
	virtual void async_start(std::function<void()>) = 0;
	virtual void async_read_some(boost::asio::mutable_buffers_1,
		std::function<void(boost::system::error_code, size_t)>) = 0;
	virtual void async_write(std::string const &,
		std::function<void(boost::system::error_code, size_t)>) = 0;
	virtual void shutdown(boost::asio::socket_base::shutdown_type type, boost::system::error_code &) = 0;
};

struct Socket : public BaseSocket
{
	Socket(boost::asio::io_service &io_service)
	: socket_(io_service)
	{}

	virtual ~Socket() {}

	void async_accept(tcp::acceptor &acceptor, std::function<void(boost::system::error_code)> f) {
		acceptor.async_accept(socket_, f);
	}

	virtual void async_start(std::function<void()> f)
	{
		// nothing to do, so immediately call it
		f();
	}

	virtual void async_read_some(boost::asio::mutable_buffers_1 b,
		std::function<void(boost::system::error_code, size_t)> f)
	{
		socket_.async_read_some(b, f);
	}

	virtual void async_write(std::string const &b,
		std::function<void(boost::system::error_code, size_t)> f)
	{
		boost::asio::async_write(socket_, boost::asio::buffer(b.data(), b.size()), f);
	}

	virtual void shutdown(boost::asio::socket_base::shutdown_type type, boost::system::error_code &ec)
	{
		socket_.shutdown(type, ec);
	}

private:
	tcp::socket socket_;
};

#ifdef SG_HTTP_SSL
struct SslSocket : public BaseSocket
{
	SslSocket(boost::asio::io_service &io_service,
	    boost::asio::ssl::context &context)
	: socket_(io_service, context)
	{}

	void async_accept(tcp::acceptor &acceptor, std::function<void(boost::system::error_code)> f) {
		acceptor.async_accept(socket_.lowest_layer(), f);
	}

	virtual void async_start(std::function<void()> f)
	{
		socket_.async_handshake(boost::asio::ssl::stream_base::server,
			[f](boost::system::error_code error) {
				if(error) {
					std::cerr << "SSL handshake error: " << error.message() << std::endl;
				} else {
					f();
				}
			}
		);
	}

	virtual void async_read_some(boost::asio::mutable_buffers_1 b,
		std::function<void(boost::system::error_code, size_t)> f)
	{
		socket_.async_read_some(b, f);
	}

	virtual void async_write(std::string const &b,
		std::function<void(boost::system::error_code, size_t)> f)
	{
		boost::asio::async_write(socket_, boost::asio::buffer(b.data(), b.size()), f);
	}

	virtual void shutdown(boost::asio::socket_base::shutdown_type type, boost::system::error_code &ec)
	{
		socket_.lowest_layer().shutdown(type, ec);
	}

private:
	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
	ssl_socket socket_;
};
#endif

}}
