#pragma once

#include <sstream>
#include "http_global.hpp"
#include "http_util.hpp"

namespace skynet {

struct HttpMessage {
	std::map<std::string, std::string> headers;

	std::string toText() {
		std::stringstream ss;
		for(auto it = headers.begin(); it != headers.end(); ++it) {
			ss << it->first << ": " << it->second << std::endl;
		}
		ss << std::endl << body();
		return ss.str();
	}

	std::string body() const {
		return body_;
	}

	void setBody(std::string body, std::string contenttype) {
		headers["Content-Type"] = contenttype;
		std::stringstream cl;
		cl << body.length();
		headers["Content-Length"] = cl.str();
		body_ = body;
	}

private:
	std::string body_;

};

struct HttpResponse : public HttpMessage {
	uint16_t status;
	std::string statusText;
	std::string httpVersion;

	HttpResponse(uint16_t status, std::string statusText, std::string httpVersion)
	: status(status), statusText(statusText), httpVersion(httpVersion) {
	}

	HttpResponse(uint16_t status)
	: status(status), statusText(statusTextFor(status)), httpVersion("HTTP/1.1") {
	}

	std::string toString() {
		std::stringstream ss;
		// HTTP/1.1 200 OK
		ss << httpVersion << " " << status << " " << statusText << std::endl;
		ss << HttpMessage::toText();
		return ss.str();
	}
};

struct HttpRequest : public HttpMessage {
	std::string method;
	std::string uri;
	std::string httpVersion;

	struct IncompleteRequestException : public std::runtime_error {
		IncompleteRequestException()
		: std::runtime_error("Incomplete request") {}
	};

	struct InvalidRequestException : public std::runtime_error {
		InvalidRequestException(std::string what)
		: std::runtime_error("Bad request: " + what) {}
	};

	HttpRequest(std::string rawHttp) {
		std::stringstream ss(rawHttp);
		std::string line;
		{ // First line: <method> <uri> <httpver>
			std::getline(ss, line);
			std::stringstream firstline(line);
			firstline >> method >> uri >> httpVersion;
			if(httpVersion != "HTTP/1.0" && httpVersion != "HTTP/1.1") {
				throw InvalidRequestException("HTTP version must be HTTP/1.0 or HTTP/1.1");
			}
		}

		// Headers:
		bool headersRead = false;
		int linenr = 1;
		while(std::getline(ss, line)) {
			linenr++;
			if(line == "" || line == "\r") {
				headersRead = true;
				break;
			}
			std::string name, value;
			bool isName = true;
			for(unsigned i = 0; i < line.length(); ++i) {
				if(i + 1 == line.length() && line[i] == '\r') {
					// skip
					break;
				} else if(isName && line[i] == ':') {
					isName = false;
				} else if(isName) {
					name += line[i];
				} else {
					value += line[i];
				}
			}
			if(isName) {
				std::stringstream ss;
				ss << "Invalid syntax on header line " << linenr;
				throw InvalidRequestException(ss.str());
			}
			headers[name] = value;
		}

		if(!headersRead) {
			throw IncompleteRequestException();
		}

		// Body: TODO
	}

	std::string toString() {
		std::stringstream ss;
		// GET /foo/bar HTTP/1.1
		ss << method << " " << urlencode(uri) << " " << httpVersion << std::endl;
		ss << HttpMessage::toText();
		return ss.str();
	}
};

}
