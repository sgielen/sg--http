#pragma once

namespace skynet {

struct BaseSocket
{
	typedef boost::asio::ip::tcp tcp;

	virtual tcp::socket &accept_socket() = 0;
	virtual void async_read_some(boost::asio::mutable_buffers_1,
		std::function<void(boost::system::error_code, size_t)>) = 0;
	virtual void async_write(std::vector<boost::asio::const_buffer>&,
		std::function<void(boost::system::error_code, size_t)>) = 0;
};

struct Socket : public BaseSocket
{
	Socket(boost::asio::io_service &io_service)
	: socket_(io_service)
	{}

	virtual tcp::socket &accept_socket() {
		return socket_;
	}

	virtual void async_read_some(boost::asio::mutable_buffers_1 b,
		std::function<void(boost::system::error_code, size_t)> f)
	{
		socket_.async_read_some(b, f);
	}

	virtual void async_write(std::vector<boost::asio::const_buffer> &b,
		std::function<void(boost::system::error_code, size_t)> f)
	{
		boost::asio::async_write(socket_, b, f);
	}

private:
	tcp::socket socket_;
};

}
