#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <catch.hpp>
#include <thread>

TEST_CASE("Abort") {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";
	std::atomic<bool> request_received(false);
	std::atomic<bool> done_with_test(false);

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr) -> sg::http::HttpResponsePtr {
		request_received = true;
		while(!done_with_test) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		auto response = std::make_shared<sg::http::HttpResponse>(200);
		response->setBody("Hello world", "text/plain");
		return response;
	};

	// Create server thread
	HttpServer *hs = new HttpServer(host, port, 1, handleRequest);
	boost::thread t([&hs, host, port, handleRequest]() {
		hs->run();
	});

	std::this_thread::sleep_for(std::chrono::seconds(1));

	// Run client in a thread as well
	HttpClient c;
	boost::thread client([&]() {
		try {
			HttpRequest request("GET", "/");
			c.do_request(request, host, port);
		} catch(std::exception &e) {
			// TODO: timeout exception only
			done_with_test = true;
		}
	});

	// allow 5 seconds for the request to arrive
	for(size_t i = 0; i < 5; ++i) {
		if(request_received) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	CHECK(request_received); // request received correctly!

	// after 2 seconds, abort the client
	std::this_thread::sleep_for(std::chrono::seconds(2));
	c.abort();

	// allow 10 seconds for the request to abort
	for(size_t i = 0; i < 10; ++i) {
		if(done_with_test) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	CHECK(done_with_test); // request aborted correctly!

	// set done_with_test to true if it wasn't yet, so the two threads finish
	done_with_test = true;

	hs->stop();
	t.join();
	client.join();
	delete hs;
}
