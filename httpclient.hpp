#pragma once

// Most of this code was copied from the Boost HTTP client example, and as
// such is licensed under the Boost Software License, version 1.0.

#include <boost/thread/thread.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/asio.hpp>
#include "http_global.hpp"
#include "httpmessages.hpp"
#include "socket.hpp"
#include "uri.hpp"

namespace sg { namespace http {

class HttpClient {
public:
	typedef boost::asio::ip::tcp tcp;

	static HttpResponse request(HttpRequest req, std::string host, std::string port,
		boost::posix_time::time_duration timeout = boost::posix_time::pos_infin)
	{
		if(req.scheme.empty()) {
			req.scheme = "http";
		}
		if(port.empty()) {
			port = req.scheme;
		}

		using boost::system::error_code;
		using boost::lambda::var;
		using boost::lambda::_1;

		boost::asio::io_service io_service;
		boost::asio::io_service::work work(io_service);

		BaseSocketPtr socket;
		boost::asio::deadline_timer deadline(io_service);
		deadline.expires_from_now(timeout);
		std::function<void(error_code)> check_deadline = [&](error_code) {
			if(deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now())
			{
				socket->close();
			} else {
				deadline.async_wait(check_deadline);
			}
		};
		check_deadline({});

		tcp::resolver resolver(io_service);
		tcp::resolver::query query(host, port);

		// async resolve & connect
		error_code ec = boost::asio::error::would_block;
		resolver.async_resolve(query, [&](error_code r_error, tcp::resolver::iterator endpoint_it) {
			if(r_error) {
				ec = r_error;
				return;
			}

			if(req.scheme == "https") {
#if defined(SG_HTTP_SSL)
				boost::asio::ssl::context context(boost::asio::ssl::context::sslv23);
				context.set_default_verify_paths();
				socket.reset(new SslSocket(io_service, context));
#else
				throw std::runtime_error("sg::http was compiled without SSL support, but a https:// URL was used");
#endif
			} else {
				socket.reset(new Socket(io_service));
			}

			socket->async_connect(endpoint_it, var(ec) = _1);
		});

		do {
			io_service.run_one();
		} while(ec == boost::asio::error::would_block);
		if(ec || !socket || !socket->is_open()) {
			throw boost::system::system_error(ec ? ec :
				boost::asio::error::operation_aborted);
		}

#if defined(SG_HTTP_SSL)
		if(req.scheme == "https") {
			SslSocket *sock = reinterpret_cast<SslSocket*>(socket.get());
			sock->set_sni_hostname(host);
			sock->set_verify_mode(boost::asio::ssl::verify_peer, boost::asio::ssl::rfc2818_verification(host));
			ec = boost::asio::error::would_block;
			sock->async_ssl_handshake(SslSocket::ssl_socket::client, var(ec) = _1);
			do {
				io_service.run_one();
			} while(ec == boost::asio::error::would_block);
			if(ec) {
				throw boost::system::system_error(ec ? ec :
					boost::asio::error::operation_aborted);
			}
		}
#endif

		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << req.toString();

		boost::asio::write(*socket, request);

		boost::asio::streambuf response;
		ec = boost::asio::error::would_block;

		boost::asio::async_read_until(*socket, response, "\n", var(ec) = _1);
		do {
			io_service.run_one();
		} while(ec == boost::asio::error::would_block);
		if(ec) {
			throw boost::system::system_error(ec);
		}

		std::string fullResponse;
		bool socket_is_connected = true;
		while(1) {
			try {
				fullResponse.append(std::istreambuf_iterator<char>(&response), {});
				HttpResponse res(fullResponse, socket_is_connected);
				return res;
			} catch(HttpMessage::IncompleteHttpMessageException &) {
				ec = boost::asio::error::would_block;
				boost::asio::async_read(*socket, response,
					boost::asio::transfer_at_least(1), var(ec) = _1);
				do {
					io_service.run_one();
				} while(ec == boost::asio::error::would_block);
				if(ec == boost::asio::error::eof) {
					socket_is_connected = false;
				} else if(ec) {
					throw boost::system::system_error(ec);
				}
			}
		}
	}
};

}}
