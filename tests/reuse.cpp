#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <catch.hpp>
#include <thread>
#include "uri.hpp"

TEST_CASE("Client reuse") {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";
	std::atomic<bool> done_with_test(false);

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr req) -> sg::http::HttpResponsePtr {
		if(req->uri == "/timeout") {
			while(!done_with_test) {
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
		auto response = std::make_shared<sg::http::HttpResponse>(200);
		response->setBody(req->uri, "text/plain");
		return response;
	};

	// Create server thread
	HttpServer *hs = new HttpServer(host, port, 1, handleRequest);
	boost::thread t([&hs, host, port, handleRequest]() {
		hs->run();
	});

	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Run client here
	HttpClient client;

	HttpRequest request("GET", "/");
	HttpResponse response = client.do_request(request, host, port);
	CHECK(response.body() == "/");
	CHECK(response.status == 200);
	CHECK(response.statusText == "OK");

	HttpRequest request2("GET", "/foobar");
	HttpResponse response2 = client.do_request(request2, host, port);
	CHECK(response2.body() == "/foobar");
	CHECK(response2.status == 200);
	CHECK(response2.statusText == "OK");

	// Time out a client
	HttpRequest request3("GET", "/timeout");
	client.set_timeout(boost::posix_time::seconds(5));
	try {
		client.do_request(request3, host, port);
	} catch(...) {}
	done_with_test = true;

	// Run another request
	HttpRequest request4("GET", "/foobar");
	HttpResponse response4 = client.do_request(request4, host, port);
	CHECK(response4.body() == "/foobar");
	CHECK(response4.status == 200);
	CHECK(response4.statusText == "OK");

	hs->stop();
	t.join();
	delete hs;
}
