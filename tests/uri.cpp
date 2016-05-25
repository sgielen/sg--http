#include <uri.hpp>
#include <catch.hpp>
#include <vector>

struct TestInput {
	std::string full_uri;
	std::string scheme;
	std::string hostname;
	std::string port;
	std::string path;

	std::string reason;
};

TEST_CASE("URI") {
	using namespace sg::http;

	std::vector<TestInput> tests {{
		{"http://sla/",          "http", "sla",     "", "/",     "Scheme and host"},
		{"http://sla:1337/vink", "http", "sla", "1337", "/vink", "Plain parse"    },
		{"http://sla:80/vink",   "http", "sla",   "80", "/vink", "Default port"   },
		// Weird cases
		{"http://sla",           "http", "sla",     "", "/",     "No path given"},
	}};

	for(auto const& test : tests){
		Uri uri{test.full_uri};
		CAPTURE(test.full_uri);
		CAPTURE(test.reason);
		CHECK(uri.scheme   == test.scheme);
		CHECK(uri.hostname == test.hostname);
		CHECK(uri.port == test.port);
		CHECK(uri.path == test.path);
	}
}

