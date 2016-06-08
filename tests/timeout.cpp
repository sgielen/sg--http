#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <catch.hpp>
#include <thread>

TEST_CASE("Timeout") {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";
	std::atomic<bool> done_with_test(false);

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr) -> sg::http::HttpResponsePtr {
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
	boost::thread client([&]() {
		try {
			// wait 5 seconds for this request to complete, then throw
			HttpRequest request("GET", "/");
			HttpClient::request(request, host, port, boost::posix_time::seconds(5));
		} catch(std::exception &e) {
			// TODO: timeout exception only
			done_with_test = true;
		}
	});

	// allow 10 seconds for the request to time out
	for(size_t i = 0; i < 10; ++i) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
		if(done_with_test) {
			break;
		}
	}

	CHECK(done_with_test); // request timed out correctly!
	
	// set done_with_test to true if it wasn't yet, so the two threads finish
	done_with_test = true;

	hs->stop();
	t.join();
	client.join();
	delete hs;
}
