#pragma once

#include <string>
#include <map>
#include <stdexcept>
#include <sstream>
#include <functional>
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

	struct NoChunksLeftException : public std::runtime_error {
		NoChunksLeftException()
		: std::runtime_error("No chunks left") {}
	};

	std::string toHeaders() const {
		std::stringstream ss;
		for(auto it = headers.begin(); it != headers.end(); ++it) {
			ss << it->first << ": " << it->second << std::endl;
		}
		ss << std::endl;
		return ss.str();
	}

	std::string toString() const {
		std::stringstream ss;
		ss << toHeaders() << body();
		return ss.str();
	}

	bool isChunked() const {
		return isChunked_;
	}

	std::string body() const {
		if(isChunked_) {
			throw std::runtime_error("body() called, but this is a chunked HttpMessage");
		}
		return body_;
	}

	std::string readChunk() {
		if(!isChunked_) {
			throw std::runtime_error("readChunk() called, but this is not a chunked HttpMessage");
		} else if(chunksDone_) {
			throw NoChunksLeftException();
		}

		try {
			std::string res = readChunk_();
			return res;
		} catch(NoChunksLeftException &e) {
			chunksDone_ = true;
			throw e;
		}
	}

	std::string &readFullBodyFromChunks() {
		if(isChunked_) {
			body_.clear();
			try {
				while(1) {
					body_ += readChunk();
				}
			} catch(NoChunksLeftException&) {
				isChunked_ = false;
			}
		}
		return body_;
	}

	void setBody(std::string body, std::string contenttype) {
		headers["Content-Type"] = contenttype;
		std::stringstream cl;
		cl << body.length();
		headers["Content-Length"] = cl.str();
		isChunked_ = false;
		chunksDone_ = false;
		body_ = body;
	}

	void setBodyChunkFunction(std::function<std::string()> function, std::string contenttype) {
		headers["Content-Type"] = contenttype;

		isChunked_ = true;
		chunksDone_ = false;
		readChunk_ = function;
	}

protected:
	void readHeadersAndBody(std::istream &ss, std::string &rawHttp, bool socket_still_readable, bool expect_http_response) {
		readHeaders(ss);
		readBody(ss, rawHttp, socket_still_readable, expect_http_response);
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
				std::stringstream errmsg;
				errmsg << "Invalid syntax on header line " << linenr;
				throw InvalidHttpMessageException(errmsg.str());
			}
			headers[name] = value;
		}

		// Headers weren't done yet (no empty line seen)
		throw IncompleteHttpMessageException();
	}

	void readBody(std::istream &ss, std::string &rawHttp, bool socket_still_readable, bool expect_http_response) {
		isChunked_ = false;
		chunksDone_ = false;

		// RFC 2616 section 4.4 gives various ways for the content length to be communicated.
		// The first is by Transfer-Encoding, the second by Content-Length, and in the case
		// of responses requests can be delimited by closing the connection. In the case of
		// a request without a Content-Length, this code assumes there is no request body.

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
		} else if(expect_http_response) {
			// No content-length header in response reading, wait until ss is closed
			if(socket_still_readable) {
				throw IncompleteHttpMessageException();
			}
			body_ = rawHttp.substr(ss.tellg());
		}
	}

	bool isChunked_ = false;
	bool chunksDone_ = false;
	std::string body_;
	std::function<std::string()> readChunk_;
};

struct HttpResponse : public HttpMessage {
	uint16_t status;
	std::string statusText;
	std::string httpVersion;

	HttpResponse(uint16_t status_, std::string statusText_, std::string httpVersion_)
	: status(status_), statusText(statusText_), httpVersion(httpVersion_) {
	}

	HttpResponse(uint16_t status_)
	: status(status_), statusText(statusTextFor(status)), httpVersion("HTTP/1.1") {
	}

	HttpResponse(std::string rawHttp, bool socket_still_readable) {
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

		readHeadersAndBody(ss, rawHttp, socket_still_readable, true);
	}

	bool isSuccess() const {
		return status >= 200 && status < 300;
	}

	std::string toHeaders() const {
		std::stringstream ss;
		// HTTP/1.1 200 OK
		ss << httpVersion << " " << status << " " << statusText << std::endl;
		ss << HttpMessage::toHeaders();
		return ss.str();
	}

	std::string toString() const {
		std::stringstream ss;
		ss << toHeaders() << body();
		return ss.str();
	}
};

struct HttpRequest : public HttpMessage {
	std::string method;
	std::string uri;
	std::string httpVersion;

	HttpRequest(std::string method_, std::string uri_)
	: method(method_), uri(uri_), httpVersion("HTTP/1.1")
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

		readHeadersAndBody(ss, rawHttp, true, false);
	}

	std::string toHeaders() const {
		std::stringstream ss;
		// GET /foo/bar HTTP/1.1
		ss << method << " " << urlencode(uri) << " " << httpVersion << std::endl;
		ss << HttpMessage::toHeaders();
		return ss.str();
	}

	std::string toString() const {
		std::stringstream ss;
		ss << toHeaders() << body();
		return ss.str();
	}
};

}}
