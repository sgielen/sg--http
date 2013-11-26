#pragma once

#include <sstream>
#include "http_global.hpp"
#include "http_util.hpp"

namespace sg { namespace http {

struct HttpMessage {
	std::map<std::string, std::string> headers;

	struct IncompleteHttpMessageException : public std::runtime_error {
		IncompleteHttpMessageException()
		: std::runtime_error("Incomplete HTTP message") {}
	};

	struct InvalidHttpMessageException : public std::runtime_error {
		InvalidHttpMessageException(std::string what)
		: std::runtime_error("Bad HTTP message: " + what) {}
	};

	std::string toText() const {
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

protected:
	void readHeadersAndBody(std::istream &ss, std::string &rawHttp) {
		readHeaders(ss);
		readBody(ss, rawHttp);
	}

private:
	void readHeaders(std::istream &ss) {
		int linenr = 1;
		std::string line;
		while(std::getline(ss, line)) {
			linenr++;
			if(line == "" || line == "\r") {
				return;
			}
			std::string name, value;
			bool isName = true;
			bool trimSpaces = false;
			for(unsigned i = 0; i < line.length(); ++i) {
				char c = line[i];
				if(i + 1 == line.length() && c == '\r') {
					// skip
					break;
				} else if(isName && c == ':') {
					isName = false;
					trimSpaces = true;
				} else if(isName) {
					name += c;
				} else if(line[i] == ' ' && trimSpaces) {
					// skip
				} else {
					trimSpaces = false;
					value += c;
				}
			}
			if(isName) {
				std::stringstream ss;
				ss << "Invalid syntax on header line " << linenr;
				throw InvalidHttpMessageException(ss.str());
			}
			headers[name] = value;
		}

		// Headers weren't done yet (no empty line seen)
		throw IncompleteHttpMessageException();
	}

	void readBody(std::istream &ss, std::string &rawHttp) {
		if(headers.find("Content-Length") != headers.end()) {
			if(headers["Content-Length"].length() > 4) {
				// TODO: this should be 413 Request Entity Too Large
				throw InvalidHttpMessageException("Content-Length is over 4 digits, refusing to process");
			}
			std::stringstream clss;
			unsigned int contentlength;
			clss << headers["Content-Length"];
			clss >> contentlength;
			if(rawHttp.length() < unsigned(contentlength + ss.tellg())) {
				throw IncompleteHttpMessageException();
			}
			body_ = rawHttp.substr(ss.tellg(), contentlength);
		}
	}

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

	HttpResponse(std::string rawHttp) {
		std::stringstream ss(rawHttp);
		std::string line;
		
		{ // First line: <httpVersion> <status> <code>
			std::getline(ss, line);
			std::stringstream firstline(line);
			firstline >> httpVersion >> status >> statusText;
			if(httpVersion != "HTTP/1.0" && httpVersion != "HTTP/1.1") {
				throw InvalidHttpMessageException("HTTP version must be HTTP/1.0 or HTTP/1.1");
			}
		}

		readHeadersAndBody(ss, rawHttp);
	}

	bool isSuccess() const {
		return status >= 200 && status < 300;
	}

	std::string toString() const {
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

	HttpRequest(std::string method, std::string uri)
	: method(method), uri(uri), httpVersion("HTTP/1.1")
	{}

	HttpRequest(std::string rawHttp) {
		std::stringstream ss(rawHttp);
		{ // First line: <method> <uri> <httpver>
			std::string line;
			char last;
			bool has_eol = false;
			while(ss.get(last)) {
				if(last == '\n') {
					has_eol = true;
					break;
				} else {
					line.push_back(last);
				}
			}
			if(!has_eol) {
				throw IncompleteHttpMessageException();
			}
			std::stringstream firstline(line);
			firstline >> method >> uri >> httpVersion;
			if(httpVersion != "HTTP/1.0" && httpVersion != "HTTP/1.1") {
				throw InvalidHttpMessageException("HTTP version must be HTTP/1.0 or HTTP/1.1");
			}
		}

		readHeadersAndBody(ss, rawHttp);
	}

	std::string toString() const {
		std::stringstream ss;
		// GET /foo/bar HTTP/1.1
		ss << method << " " << urlencode(uri) << " " << httpVersion << std::endl;
		ss << HttpMessage::toText();
		return ss.str();
	}
};

}}
