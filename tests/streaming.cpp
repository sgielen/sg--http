#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <catch.hpp>
#include <thread>

TEST_CASE("Streaming self-connect") {
	using namespace sg::http;

	std::string host = "127.0.0.1";
	std::string port = "1337";

	int bodychunk = 0;

	// request handler
	auto handleRequest = [&](sg::http::HttpRequestPtr) -> sg::http::HttpResponsePtr {
		auto response = std::make_shared<sg::http::HttpResponse>(200);
		response->setBodyChunkFunction([&]{
			if(bodychunk == 10) {
				throw sg::http::HttpMessage::NoChunksLeftException();
			}
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::stringstream ss;
			ss << ++bodychunk << std::endl;
			return ss.str();
		}, "text/plain");
		return response;
	};

	// Create server thread
	HttpServer *hs = new HttpServer(host, port, 1, handleRequest);
	boost::thread t([&hs, host, port, handleRequest]() {
		hs->run();
	});

	// Run client here
	HttpRequest request("GET", "/");
	HttpClient client;
	size_t num_calls = 0;
	size_t last_num_bytes = 0;
	size_t total_bytes = 0;
	client.set_progress_handler([&](size_t bytes) {
		// TODO: currently, we don't send in chunked transfer mode, we
		// just stream bytes as they go. This means the number of bytes
		// received here does not include the chunk size per chunk.
		if(num_calls == 0) {
			CHECK(bytes > 30); /* response header */
		} else if(num_calls == 10) {
			CHECK(bytes == 3); /* "10\n" */
		} else if(num_calls == 11) {
			CHECK(bytes == 0); /* no more data */
		} else {
			CHECK(bytes == 2); /* just the number and the newline */
		}
		total_bytes += bytes;
		last_num_bytes = bytes;
		++num_calls;
	});

	HttpResponse response = client.do_request(request, host, port);
	CHECK(response.toString().size() == total_bytes);
	CHECK(num_calls == 12);
	CHECK(response.body() == "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n");

	hs->stop();
	t.join();
	delete hs;
}
