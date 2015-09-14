#include <uri.hpp>
#include <sg_test.hpp>
#include <vector>

struct TestInput {
	std::string full_uri;
	std::string scheme;
	std::string hostname;
	std::string port;
	std::string path;

	std::string reason;
};

int main() {
	using namespace sg::http;

	std::vector<TestInput> tests {{
		{"http://sla/",          "http", "sla",     "", "/",     "Scheme and host"},
		{"http://sla:1337/vink", "http", "sla", "1337", "/vink", "Plain parse"    },
		{"http://sla:80/vink",   "http", "sla",   "80", "/vink", "Default port"   },
	}};

	sg::test::Test tester(tests.size() * 4);

	for(auto const& test : tests){
		Uri uri{test.full_uri};
		tester.test(uri.scheme   == test.scheme,   test.reason);
		tester.test(uri.hostname == test.hostname, test.reason);
		tester.test(uri.port == test.port, test.reason);
		tester.test(uri.path == test.path, test.reason);
	}
}

