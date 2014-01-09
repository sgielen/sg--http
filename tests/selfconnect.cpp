#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <unistd.h>
#include <sg_test.hpp>

const std::string method = "OPTIONS";
const std::string uri = "/foo/bar";
const std::string uri2 = "/baz";
const std::string body = "Hello world";
const std::string contentType = "text/plain";
const std::string errorBody = "Invalid method/uri";

int main() {
	using namespace sg::http;
	sg::test::Test tester(4);

	std::string host = "127.0.0.1";
	std::string port = "1337";

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
	HttpServer *hs = new HttpServer(host, port, 1, handleRequest);
	boost::thread t([&hs, host, port, handleRequest]() {
		hs->run();
	});

	sleep(1);
	// Run client here
	HttpRequest request(method, uri);
	HttpResponse response = HttpClient::request(request, host, port);
	tester.test(response.body() == body, "Body matches");
	tester.test(response.headers["Content-Type"] == contentType, "Content-type matches");

	HttpRequest request2(method, uri2);
	HttpResponse response2 = HttpClient::request(request2, host, port);
	tester.test(response2.body().substr(0, errorBody.length()) == errorBody, "Error body matches");
	tester.test(response2.status == 400, "Status code matches");

	hs->stop();
	t.join();
	delete hs;
}
