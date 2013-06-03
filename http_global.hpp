#pragma once

#include <memory>
#include <boost/shared_ptr.hpp>

namespace skynet {

// This MUST be a boost::shared_ptr because of boosts' enable_shared_from_this
// in Connection...
class HttpConnection;
typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;

struct HttpRequest;
typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

struct HttpResponse;
typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

class HttpServerDelegate {
public:
	// Caution: HttpServer is threaded, so this method may be called
	// in various threads simultaneously.
	virtual HttpResponsePtr handleRequest(HttpRequestPtr &req) = 0;
};

typedef std::shared_ptr<HttpServerDelegate> HttpServerDelegatePtr;

struct BaseSocket;
typedef std::shared_ptr<BaseSocket> BaseSocketPtr;

}
