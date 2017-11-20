#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <catch.hpp>
#include <thread>
#include "uri.hpp"

TEST_CASE("Socket closed during request handling") {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";
	size_t requested = 0;
	size_t num_threads = 1000;

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr) -> sg::http::HttpResponsePtr {
		requested++;
		size_t sleep_milliseconds = rand() % 190 + 10;
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_milliseconds));
		return std::make_shared<sg::http::HttpResponse>(204);
	};

	// Create server thread
	HttpServer *hs = new HttpServer(host, port, 1, handleRequest);
	boost::thread t([&hs, host, port, handleRequest]() {
		hs->run();
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Run a client, and let it time-out quickly. This used to crash occasionally,
	// so do it many times.
	std::vector<boost::thread> threads;
	for(size_t i = 0; i < num_threads; ++i) {
		threads.emplace_back([&]{
			try {
				HttpRequest request("GET", "/");
				HttpClient::request(request, host, port, boost::posix_time::milliseconds(1));
			} catch(std::exception&) {
				// TODO: timeout exception only
			}
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	for(auto &thr : threads) {
		thr.join();
	}

	hs->stop();
	t.join();
	delete hs;
}
