#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <catch.hpp>
#include <thread>
#include "uri.hpp"

const std::string method = "OPTIONS";
const std::string uri = "/foo/bar";
const std::string uri2 = "/baz";
const std::string body = "Hello world";
const std::string contentType = "text/plain";
const std::string errorBody = "Invalid method/uri";
const sg::http::Uri uri3{"http://example.org:1337/foo/bar"};
const std::string expected_host = "example.org:1337";

TEST_CASE("Self-connect") {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";
	bool expect_host_header = false;

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr req) -> sg::http::HttpResponsePtr {
		if(req->method != method || req->uri != uri) {
			throw sg::http::HttpBadRequest(req, errorBody);
		}
		if(expect_host_header && req->headers["Host"] != expected_host) {
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

	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Run client here
	HttpRequest request(method, uri);
	HttpResponse response = HttpClient::request(request, host, port);
	CHECK(response.body() == body);
	CHECK(response.headers["Content-Type"] == contentType);
	CHECK(response.status == 200);
	CHECK(response.statusText == "OK");

	HttpRequest request2(method, uri2);
	HttpResponse response2 = HttpClient::request(request2, host, port);
	CHECK(response2.body().substr(0, errorBody.length()) == errorBody);
	CHECK(response2.status == 400);
	CHECK(response2.statusText == "Bad Request");

	expect_host_header = true;
	HttpRequest request3(method, uri3);
	HttpResponse response3 = HttpClient::request(request3, host, port);
	CHECK(response3.body() == body);
	CHECK(response3.status == 200);
	CHECK(response3.statusText == "OK");

	hs->stop();
	t.join();
	delete hs;
}
