#pragma once

#include "httpserver.hpp"
#include <boost/regex.hpp>

namespace sg { namespace http {

struct ApiRoute {
	boost::regex uri_regex;
	std::vector<std::string> acceptable_methods;
	std::function<HttpResponsePtr(HttpRequestPtr, std::vector<std::string>)> handler;
};

struct ApiRouter {
	ApiRouter() = default;
	ApiRouter(ApiRoute route) {
		addRoute(route);
	}
	ApiRouter(std::vector<ApiRoute> routes) : routes(routes) {
	}

	void addRoute(ApiRoute const &r) {
		routes.push_back(r);
	}

	HttpResponsePtr handle(HttpRequestPtr request) const {
		return request_exception_wrapper(request, [this](HttpRequestPtr request) -> HttpResponsePtr {
			return handle_throws(request);
		});
	}

	HttpResponsePtr handle_throws(HttpRequestPtr request) const {
		bool found_uri_match = false;
		for(const ApiRoute &route : routes) {
			boost::smatch match;
			if(!regex_match(request->uri, match, route.uri_regex)) {
				// URI does not match
				continue;
			}
			if(!route.acceptable_methods.empty() && !contains(route.acceptable_methods, request->method)) {
				// method does not match (note that URI did match)
				found_uri_match = true;
				continue;
			}
			std::vector<std::string> captures;
			// smatch begins with the full match, we only want the captures, hence + 1
			std::copy(match.begin() + 1, match.end(), captures.begin());
			return route.handler(request, captures);
		}

		// No handler was found to handle this request
		if(found_uri_match) {
			throw HttpMethodNotAcceptable(request);
		} else {
			throw HttpNotFound(request);
		}
	}

private:
	std::vector<ApiRoute> routes;
};

class ApiServer : public HttpServer {
public:
	ApiServer(std::string address, std::string port,
		size_t thread_pool_size, SslContext context)
	: HttpServer(address, port, thread_pool_size, std::move(context), handler())
	{}

	ApiServer(std::string address, std::string port,
		size_t thread_pool_size)
	: HttpServer(address, port, thread_pool_size, handler())
	{}

	void addRoute(ApiRoute const &r) {
		router.addRoute(r);
	}

	void addRoutes(std::vector<ApiRoute> const &rv) {
		for(ApiRoute const &r : rv) {
			router.addRoute(r);
		}
	}

private:
	RequestHandler handler() {
		return [&](HttpRequestPtr request) -> HttpResponsePtr {
			return router.handle(request);
		};
	}

	ApiRouter router;
};

}}
