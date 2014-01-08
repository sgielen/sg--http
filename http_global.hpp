#pragma once

#include <memory>
#include <boost/shared_ptr.hpp>

namespace sg { namespace http {

// This MUST be a boost::shared_ptr because of boosts' enable_shared_from_this
// in Connection...
class HttpConnection;
typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;

struct HttpRequest;
typedef std::shared_ptr<HttpRequest> HttpRequestPtr;

struct HttpResponse;
typedef std::shared_ptr<HttpResponse> HttpResponsePtr;

// Caution: HttpServer is threaded, so this function may be called in various
// threads simultaneously.
typedef std::function<HttpResponsePtr(HttpRequestPtr)> RequestHandler;

struct BaseSocket;
typedef std::shared_ptr<BaseSocket> BaseSocketPtr;

}}
