#pragma once

#include <string>
#include <map>
#include <stdexcept>
#include <sstream>
#include <functional>
#include "http_global.hpp"
#include "http_util.hpp"
#include "uri.hpp"

namespace sg { namespace http {

struct HttpMessage {
	std::map<std::string, std::string> headers;

	struct IncompleteHttpMessageException : public std::runtime_error {
		IncompleteHttpMessageException()
		: std::runtime_error("Incomplete HTTP message") {}
	};

	struct InvalidHttpMessageException : public std::runtime_error {
		InvalidHttpMessageException(std::string w)
		: std::runtime_error("Bad HTTP message: " + w) {}
	};

	struct NoChunksLeftException : public std::runtime_error {
		NoChunksLeftException()
		: std::runtime_error("No chunks left") {}
	};

	std::string toHeaders() const {
		std::stringstream ss;
		for(auto it = headers.begin(); it != headers.end(); ++it) {
			ss << it->first << ": " << it->second << "\r\n";
		}
		ss << "\r\n";
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

	void setBody(std::string b, std::string contenttype) {
		headers["Content-Type"] = contenttype;
		std::stringstream cl;
		cl << b.length();
		headers["Content-Length"] = cl.str();
		isChunked_ = false;
		chunksDone_ = false;
		body_ = b;
	}

	void setBodyChunkFunction(std::function<std::string()> function, std::string contenttype) {
		headers["Content-Type"] = contenttype;

		isChunked_ = true;
		chunksDone_ = false;
		readChunk_ = function;
	}

protected:
	void readHeadersAndBody(std::string const &rawHttp, size_t &pos, bool socket_still_readable, bool expect_http_response) {
		readHeaders(rawHttp, pos);
		readBody(rawHttp, pos, socket_still_readable, expect_http_response);
	}

private:
	void readHeaders(std::string const &http, size_t &pos) {
		int linenr = 1;
		while(pos < http.length()) {
			std::string line = read_line(http, pos);
			linenr++;
			if(line == "\n" || line == "\r\n") {
				return;
			}
			std::string name, value;
			bool isName = true;
			bool trimSpaces = false;
			for(unsigned i = 0; i < line.length(); ++i) {
				char c = line[i];
				if(i + 2 == line.length() && c == '\r' && line[i+1] == '\n') {
					// skip
					break;
				} else if(i + 1 == line.length() && c == '\n') {
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

	void readBody(std::string const &rawHttp, size_t &pos, bool socket_still_readable, bool expect_http_response) {
		isChunked_ = false;
		chunksDone_ = false;

		// RFC 2616 section 4.4 gives various ways for the content length to be communicated.
		// The first is by Transfer-Encoding, the second by Content-Length, and in the case
		// of responses requests can be delimited by closing the connection. In the case of
		// a request without a Content-Length, this code assumes there is no request body.

		if(headers.find("Content-Length") != headers.end()) {
			if(!expect_http_response && headers["Content-Length"].length() > 7) {
				// TODO: this should be 413 Request Entity Too Large
				// TODO: also, the max size should be configurable
				throw InvalidHttpMessageException("Content-Length is too long (" + headers["Content-Length"] + "), refusing to process");
			}
			std::stringstream clss;
			unsigned int contentlength;
			clss << headers["Content-Length"];
			clss >> contentlength;
			if(rawHttp.length() < unsigned(contentlength + pos)) {
				throw IncompleteHttpMessageException();
			}
			body_ = rawHttp.substr(pos, contentlength);
		} else if(expect_http_response) {
			// No content-length header in response reading, wait until ss is closed
			if(socket_still_readable) {
				throw IncompleteHttpMessageException();
			}
			body_ = rawHttp.substr(pos);
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

	HttpResponse(std::string const &rawHttp, bool socket_still_readable) {
		size_t pos = 0;
		
		{ // First line: <httpVersion> <status> <code>
			std::string line = read_line(rawHttp, pos);
			std::stringstream firstline(line);
			firstline >> httpVersion >> status;
			statusText = line.substr(uint32_t(firstline.tellg()) + 1);
			while(!statusText.empty() && isspace(statusText.back())) {
				statusText.resize(statusText.size() - 1);
			}
			if(httpVersion != "HTTP/1.0" && httpVersion != "HTTP/1.1") {
				throw InvalidHttpMessageException("HTTP version must be HTTP/1.0 or HTTP/1.1");
			}
		}

		readHeadersAndBody(rawHttp, pos, socket_still_readable, true);
	}

	bool isSuccess() const {
		return status >= 200 && status < 300;
	}

	std::string toHeaders() const {
		std::stringstream ss;
		// HTTP/1.1 200 OK
		ss << httpVersion << " " << status << " " << statusText << "\r\n";
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
	std::string scheme;
	std::string uri;
	std::string httpVersion;

	HttpRequest(std::string method_, std::string scheme_, std::string uri_)
	: method(method_), scheme(scheme_), uri(uri_), httpVersion("HTTP/1.1")
	{}

	HttpRequest(std::string method_, std::string uri_)
	: HttpRequest(method_, "http", uri_)
	{}

	HttpRequest(std::string m, sg::http::Uri uri_)
	: HttpRequest(m, uri_.scheme, uri_.toPathString())
	{
		headers["Host"] = uri_.hostname;
		if(!uri_.port.empty()) {
			headers["Host"] += ":" + uri_.port;
		}
	}

	HttpRequest(std::string const &rawHttp) {
		size_t pos = 0;
		std::stringstream ss(rawHttp);
		{ // First line: <method> <uri> <httpver>
			std::string line = read_line(rawHttp, pos);
			if(line[line.length() - 1] != '\n') {
				throw IncompleteHttpMessageException();
			}
			std::stringstream firstline(line);
			firstline >> method >> uri >> httpVersion;
			if(httpVersion != "HTTP/1.0" && httpVersion != "HTTP/1.1") {
				throw InvalidHttpMessageException("HTTP version must be HTTP/1.0 or HTTP/1.1");
			}
		}

		readHeadersAndBody(rawHttp, pos, true, false);
	}

	std::string toHeaders() const {
		std::stringstream ss;
		// GET /foo/bar HTTP/1.1
		ss << method << " " << urlencode(uri) << " " << httpVersion << "\r\n";
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
