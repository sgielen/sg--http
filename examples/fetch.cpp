#include <httpserver.hpp>
#include <httpclient.hpp>
#include <httpmessages.hpp>

using namespace sg::http;

int main(int argc, char *argv[]) {
	if(argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <URL>" << std::endl;
		return 1;
	}

	Uri uri(argv[1]);

	HttpRequest request("GET", uri);
	HttpResponse response = HttpClient::request(request, uri.hostname, uri.port);

	if(response.isChunked()) {
		try {
			while(1) {
				std::cout << response.readChunk();
			}
		} catch(HttpMessage::NoChunksLeftException&) {}
	} else {
		std::cout << response.body();
	}
}
