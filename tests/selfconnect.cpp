#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <unistd.h>

const std::string method = "OPTIONS";
const std::string uri = "/foo/bar";
const std::string uri2 = "/baz";
const std::string body = "Hello world";
const std::string contentType = "text/plain";
const std::string errorBody = "Invalid method/uri";

int main() {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";

	std::cout << "1..2" << std::endl;

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr req) -> sg::http::HttpResponsePtr {
		if(req->method != method || req->uri != uri) {
			throw sg::http::HttpBadRequest(req, errorBody);
		}
		auto response = std::make_shared<sg::http::HttpResponse>(200);
		response->setBody(body, contentType);
		return response;
	};

	// Create server thread
	boost::thread t([host, port, handleRequest]() {
		HttpServer hs(host, port, 1, handleRequest);
		hs.run();
	});

	sleep(1);
	// Run client here
	HttpRequest request(method, uri);
	HttpResponse response = HttpClient::request(request, host, port);
	if(response.body() == body && response.headers["Content-Type"] == contentType) {
		std::cout << "ok 1 Normal request" << std::endl;
	} else {
		std::cout << "not ok 1 Normal request" << std::endl;
	}

	HttpRequest request2(method, uri2);
	HttpResponse response2 = HttpClient::request(request2, host, port);
	if(response2.body().substr(0, errorBody.length()) == errorBody && response2.status == 400) {
		std::cout << "ok 2 Error request" << std::endl;
	} else {
		std::cout << "not ok 2 Error request" << std::endl;
	}
}
