#include <apiserver.hpp>
#include <unistd.h>
#include <sg_test.hpp>

int main() {
	using namespace sg::http;
	sg::test::Test tester(11);

	typedef HttpRequestPtr HR;
	typedef std::vector<std::string> PM; // parameters

	{ // Handle a simple request
		bool ok = false;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {"GET"}, [&](HR, PM) -> HttpResponsePtr {
			ok = true;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/foo"));
		tester.test(resp->status == 204, "Got correct response");
		tester.test(ok, "Request was handled");
	}

	{ // Handle a simple any-method request
		bool ok = false;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {}, [&](HR, PM) -> HttpResponsePtr {
			ok = true;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("FOOBAR", "/foo"));
		tester.test(resp->status == 204, "Got correct response");
		tester.test(ok, "Request was handled");
	}

	{ // Two methods, handle the right one
		bool ok = false;
		ApiRouter r;
		r.addRoute(ApiRoute{boost::regex{"/foo"}, {"GET"}, [&](HR, PM) -> HttpResponsePtr {
			ok = false;
			return std::make_shared<HttpResponse>(500);
		}});
		r.addRoute(ApiRoute{boost::regex{"/foo"}, {"POST"}, [&](HR, PM) -> HttpResponsePtr {
			ok = true;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("POST", "/foo"));
		tester.test(resp->status == 204, "Got correct response");
		tester.test(ok, "Request was handled");
	}

	{ // No methods matched
		bool ok = true;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {"GET", "POST"}, [&](HR, PM) -> HttpResponsePtr {
			ok = false;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("FOOBAR", "/foo"));
		tester.test(resp->status == 405, "Got correct response");
		tester.test(ok, "Request was not handled");
	}

	{ // No paths matched
		bool ok = true;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {}, [&](HR, PM) -> HttpResponsePtr {
			ok = false;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/bar"));
		tester.test(resp->status == 404, "Got correct response");
		tester.test(ok, "Request was not handled");
	}

	{ // Empty router: no paths matched
		ApiRouter r;
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/bar"));
		tester.test(resp->status == 404, "Got correct response");
	}
}
