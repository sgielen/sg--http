#include <httpserver.hpp>
#include <httpclient.hpp>
#include <boost/thread.hpp>
#include <unistd.h>
#include <sg_test.hpp>

int main() {
	using namespace sg::http;
	sg::test::Test tester(1);

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
			sleep(1);
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
	tester.test(response.body() == "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n", "Body matches");

	hs->stop();
	t.join();
	delete hs;
}
