#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <catch.hpp>
#include <thread>
#include "uri.hpp"

TEST_CASE("Regression in responses without a body") {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr) -> sg::http::HttpResponsePtr {
		return std::make_shared<sg::http::HttpResponse>(204);
	};

	// Create server thread
	HttpServer *hs = new HttpServer(host, port, 1, handleRequest);
	boost::thread t([&hs, host, port, handleRequest]() {
		hs->run();
	});

	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Run client here
	HttpRequest request("GET", "/");
	HttpResponse response = HttpClient::request(request, host, port);
	CHECK(response.body() == "");
	CHECK(response.status == 204);
	CHECK(response.statusText == "No Content");

	hs->stop();
	t.join();
	delete hs;
}
