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


TEST_CASE("URI queries") {
	using namespace sg::http;

	SECTION("no query") {
		std::string full_uri("http://localhost/");
		Uri uri(full_uri);
		CAPTURE(full_uri);
		CAPTURE(uri.query);
		auto queryParams = uri.queryParameters();
		CHECK(queryParams.size() == 0);
	}

	SECTION("simple URL") {
		std::string full_uri("http://localhost/?quux=1236&mumble=mamble");
		Uri uri(full_uri);
		CAPTURE(full_uri);
		CAPTURE(uri.query);
		auto queryParams = uri.queryParameters();
		CHECK(queryParams.size() == 2);
		CHECK(queryParams.at("quux") == "1236");
		CHECK(queryParams.at("mumble") == "mamble");
	}

	SECTION("complex URL") {
		std::string full_uri = "http://foo_bar:1234/baz?quux=1238&mumble=memble&empty&empty2=";
		Uri uri(full_uri);
		CAPTURE(full_uri);
		CAPTURE(uri.query);
		auto queryParams = uri.queryParameters();
		CHECK(queryParams.size() == 4);
		CHECK(queryParams.at("quux") == "1238");
		CHECK(queryParams.at("mumble") == "memble");
		CHECK(queryParams.at("empty") == "");
		CHECK(queryParams.at("empty2") == "");
	}

	SECTION("more complex URL") {
		std::string full_uri = "https://1.2.3.4:5678/baz/path?quux=1240&mumble=momble&empty&empty2=&#location";
		Uri uri(full_uri);
		CAPTURE(full_uri);
		CAPTURE(uri.query);
		auto queryParams = uri.queryParameters();
		CHECK(queryParams.size() == 4);
		CHECK(queryParams.at("quux") == "1240");
		CHECK(queryParams.at("mumble") == "momble");
		CHECK(queryParams.at("empty") == "");
		CHECK(queryParams.at("empty2") == "");
	}
}
