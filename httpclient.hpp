#pragma once

// Most of this code was copied from the Boost HTTP client example, and as
// such is licensed under the Boost Software License, version 1.0.

#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include "http_global.hpp"
#include "httpmessages.hpp"
#include "uri.hpp"

namespace sg { namespace http {

class HttpClient {
public:
	typedef boost::asio::ip::tcp tcp;

	static HttpResponse request(HttpRequest req, std::string host, std::string port) {
		if(req.scheme.empty()) {
			req.scheme = "http";
		}
		if(port.empty()) {
			port = req.scheme;
		}

		boost::asio::io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::resolver::query query(host, port);
		tcp::resolver::iterator endpoint_it = resolver.resolve(query);

		BaseSocketPtr socket;
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

		// Try every possible endpoint until we can connect to one
		socket->connect(endpoint_it);

#if defined(SG_HTTP_SSL)
		if(req.scheme == "https") {
			SslSocket *sock = reinterpret_cast<SslSocket*>(socket.get());
			sock->set_sni_hostname(host);
			sock->set_verify_mode(boost::asio::ssl::verify_peer, boost::asio::ssl::rfc2818_verification(host));
			sock->ssl_handshake(SslSocket::ssl_socket::client);
		}
#endif

		boost::asio::streambuf request;
		std::ostream request_stream(&request);
		request_stream << req.toString();

		boost::asio::write(*socket, request);

		boost::asio::streambuf response;
		boost::asio::read_until(*socket, response, "\n");

		std::string fullResponse;
		bool socket_is_connected = true;
		while(1) {
			try {
				std::istream is(&response);
				char buffer[512];
				while(is.read(buffer, sizeof(buffer))) {
					fullResponse.append(buffer, sizeof(buffer));
				}
				if(is.gcount() > 0) {
					fullResponse.append(buffer, is.gcount());
				}
				HttpResponse res(fullResponse, socket_is_connected);
				return res;
			} catch(HttpMessage::IncompleteHttpMessageException &) {
				boost::system::error_code error;
				boost::asio::read(*socket, response,
					boost::asio::transfer_at_least(1), error);
				if(error == boost::asio::error::eof) {
					socket_is_connected = false;
				} else if(error) {
					throw boost::system::system_error(error);
				}
			}
		}
		abort();
	}
};

}}
