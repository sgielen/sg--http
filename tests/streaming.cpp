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
	HttpResponse response = HttpClient::request(request, host, port);
	CHECK(response.body() == "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n");

	hs->stop();
	t.join();
	delete hs;
}
