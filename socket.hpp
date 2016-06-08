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
	typedef boost::asio::detail::consuming_buffers<boost::asio::const_buffer, boost::asio::const_buffers_1> const_buffers;

	virtual ~BaseSocket() {}

	virtual void async_connect(boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::iterator&,
		std::function<void(boost::system::error_code, tcp::resolver::iterator)>) = 0;
	virtual void async_accept(tcp::acceptor&, std::function<void(boost::system::error_code)>) = 0;
	virtual void async_start(std::function<void()>) = 0;
	virtual void async_read_some(boost::asio::mutable_buffers_1,
		std::function<void(boost::system::error_code, size_t)>) = 0;
	virtual void async_write(std::string const &,
		std::function<void(boost::system::error_code, size_t)>) = 0;

	virtual void connect(boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::iterator&) = 0;
	virtual size_t write_some(const_buffers, boost::system::error_code&) = 0;
	virtual size_t read_some(boost::asio::mutable_buffers_1, boost::system::error_code&) = 0;
	virtual void shutdown(boost::asio::socket_base::shutdown_type type, boost::system::error_code &) = 0;

	virtual bool is_open() = 0;
	virtual void close() = 0;
};

struct Socket : public BaseSocket
{
	Socket(boost::asio::io_service &io_service)
	: socket_(io_service)
	{}

	virtual ~Socket() {}

	void async_connect(boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::iterator &it,
		std::function<void(boost::system::error_code, tcp::resolver::iterator)> f) {
		boost::asio::async_connect(socket_, it, f);
	}

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

	virtual void connect(boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::iterator &it)
	{
		boost::asio::connect(socket_, it);
	}

	virtual size_t write_some(const_buffers buf, boost::system::error_code &ec)
	{
		return socket_.write_some(buf, ec);
	}

	virtual size_t read_some(boost::asio::mutable_buffers_1 buf, boost::system::error_code &ec)
	{
		return socket_.read_some(buf, ec);
	}

	virtual void shutdown(boost::asio::socket_base::shutdown_type type, boost::system::error_code &ec)
	{
		socket_.shutdown(type, ec);
	}

	virtual bool is_open()
	{
		return socket_.is_open();
	}

	virtual void close()
	{
		socket_.close();
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

	void async_connect(boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::iterator &it,
		std::function<void(boost::system::error_code, tcp::resolver::iterator)> f) {
		boost::asio::async_connect(socket_.lowest_layer(), it, f);
	}

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

	virtual void connect(boost::asio::ip::basic_resolver<boost::asio::ip::tcp>::iterator &it)
	{
		boost::asio::connect(socket_.lowest_layer(), it);
	}

	virtual size_t write_some(const_buffers buf, boost::system::error_code &ec)
	{
		return socket_.write_some(buf, ec);
	}

	virtual size_t read_some(boost::asio::mutable_buffers_1 buf, boost::system::error_code &ec)
	{
		return socket_.read_some(buf, ec);
	}

	virtual void shutdown(boost::asio::socket_base::shutdown_type type, boost::system::error_code &ec)
	{
		socket_.lowest_layer().shutdown(type, ec);
	}

	void set_sni_hostname(std::string hostname) {
		SSL_set_tlsext_host_name(socket_.native_handle(), hostname.c_str());
	}

	template <typename VerifyCallback>
	void set_verify_mode(int mode, VerifyCallback callback) {
		socket_.set_verify_mode(mode);
		socket_.set_verify_callback(callback);
	}

	void ssl_handshake(boost::asio::ssl::stream_base::handshake_type mode) {
		socket_.handshake(mode);
	}

	void async_ssl_handshake(boost::asio::ssl::stream_base::handshake_type mode,
		std::function<void(boost::system::error_code)> f) {
		socket_.async_handshake(mode, f);
	}

	virtual bool is_open()
	{
		return socket_.lowest_layer().is_open();
	}

	virtual void close()
	{
		socket_.lowest_layer().close();
	}

	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
private:
	ssl_socket socket_;
};
#endif

}}
