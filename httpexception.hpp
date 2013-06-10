#pragma once

#include <stdexcept>
#include <map>
#include <string>
#include "http_util.hpp"

namespace skynet {

class HttpException : public std::runtime_error {
	uint16_t code_;
	HttpRequestPtr req_;

protected:
	std::map<std::string, std::string> headers_;

public:
	HttpException(uint16_t code, HttpRequestPtr req, std::string what)
	: std::runtime_error(what)
	, code_(code)
	, req_(req) {}

	HttpException(uint16_t code, HttpRequestPtr req)
	: std::runtime_error(statusTextFor(code))
	, code_(code)
	, req_(req) {}

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
	HttpExceptionTempl(HttpRequestPtr req, std::string what)
	: HttpException(Code, req, what) {}
	HttpExceptionTempl(HttpRequestPtr req)
	: HttpException(Code, req) {}
};

typedef HttpExceptionTempl<400> HttpBadRequest;
// HTTP 401 has a nonstandard implementation below
typedef HttpExceptionTempl<404> HttpNotFound;
typedef HttpExceptionTempl<405> HttpMethodNotAcceptable;
typedef HttpExceptionTempl<500> HttpInternalServerError;

class HttpUnauthorized : public HttpException {
	void add_auth_header() {
		headers_["WWW-Authenticate"] = "Basic realm=\"Skynet\"";
	}

public:
	HttpUnauthorized(HttpRequestPtr req, std::string what)
	: HttpException(401, req, what) {
		add_auth_header();
	}
	HttpUnauthorized(HttpRequestPtr req)
	: HttpException(401, req) {
		add_auth_header();
	}
};

}
