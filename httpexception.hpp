#pragma once

#include <stdexcept>
#include <map>
#include <string>
#include "http_util.hpp"

namespace sg { namespace http {

class HttpException : public std::runtime_error {
	uint16_t code_;
	HttpRequestPtr req_;

protected:
	std::map<std::string, std::string> headers_;

public:
	HttpException(uint16_t c, HttpRequestPtr r, std::string w)
	: std::runtime_error(w)
	, code_(c)
	, req_(r) {}

	HttpException(uint16_t c, HttpRequestPtr r)
	: std::runtime_error(statusTextFor(c))
	, code_(c)
	, req_(r) {}

	std::map<std::string, std::string> headers() const {
		return headers_;
	}

	uint16_t code() const {
		return code_;
	}

	HttpRequestPtr req() const {
		return req_;
	}

	std::string body() const {
		return std::string(what()) + "\n\n"
		    + "Method: " + req_->method + "\n"
		    + "Path:   " + req_->uri;
	}
};

template <uint16_t Code>
class HttpExceptionTempl : public HttpException {
public:
	HttpExceptionTempl(HttpRequestPtr r, std::string w)
	: HttpException(Code, r, w) {}
	HttpExceptionTempl(HttpRequestPtr r)
	: HttpException(Code, r) {}
};

typedef HttpExceptionTempl<400> HttpBadRequest;
// HTTP 401 has a nonstandard implementation below
typedef HttpExceptionTempl<404> HttpNotFound;
typedef HttpExceptionTempl<405> HttpMethodNotAcceptable;
typedef HttpExceptionTempl<500> HttpInternalServerError;

class HttpUnauthorized : public HttpExceptionTempl<401> {
	typedef HttpExceptionTempl<401> Parent;
	void add_auth_header(std::string realm) {
		// TODO: what characters are allowed in the realm? how to escape others?
		headers_["WWW-Authenticate"] = "Basic realm=\"" + realm + "\"";
	}

public:
	HttpUnauthorized(HttpRequestPtr r) : Parent(r) {}
	HttpUnauthorized(HttpRequestPtr r, std::string w) : Parent(r, w) {}
	HttpUnauthorized(HttpRequestPtr r, std::string w, std::string realm) : Parent(r, w) {
		add_auth_header(realm);
	}
};

inline HttpResponsePtr request_exception_wrapper(HttpRequestPtr &request, RequestHandler handler) {
	try {
		return handler(request);
	} catch(HttpException &e) {
		auto response = HttpResponsePtr(new HttpResponse(e.code()));
		response->headers = e.headers();
		response->setBody(e.body(), "text/plain");
		return response;
	} catch(std::exception &e) {
		auto response = HttpResponsePtr(new HttpResponse(500));
		response->setBody("Internal server error: " + std::string(e.what()), "text/plain");
		return response;
	} catch(...) {
		auto response = HttpResponsePtr(new HttpResponse(500));
		response->setBody("Internal server error", "text/plain");
		return response;
	}
}

}}
