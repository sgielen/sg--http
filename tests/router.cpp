#include <apiserver.hpp>
#include <unistd.h>
#include <catch.hpp>

using namespace sg::http;

typedef HttpRequestPtr HR;
typedef std::vector<std::string> PM; // parameters

TEST_CASE("Handle a simple request") {
	bool ok = false;
	ApiRouter r(ApiRoute{boost::regex{"/foo"}, {"GET"}, [&](HR, PM) -> HttpResponsePtr {
		ok = true;
		return std::make_shared<HttpResponse>(204);
	}});
	HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/foo"));
	REQUIRE(resp->status == 204);
	REQUIRE(ok);
}

TEST_CASE("Handle a simple any-method request") {
	bool ok = false;
	ApiRouter r(ApiRoute{boost::regex{"/foo"}, {}, [&](HR, PM) -> HttpResponsePtr {
		ok = true;
		return std::make_shared<HttpResponse>(204);
	}});
	HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("FOOBAR", "/foo"));
	REQUIRE(resp->status == 204);
	REQUIRE(ok);
}

TEST_CASE("Two methods, handle the right one") {
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
	REQUIRE(resp->status == 204);
	REQUIRE(ok);
}

TEST_CASE("No methods matched") {
	bool ok = true;
	ApiRouter r(ApiRoute{boost::regex{"/foo"}, {"GET", "POST"}, [&](HR, PM) -> HttpResponsePtr {
		ok = false;
		return std::make_shared<HttpResponse>(204);
	}});
	HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("FOOBAR", "/foo"));
	REQUIRE(resp->status == 405);
	REQUIRE(ok);
}

TEST_CASE("No paths matched") {
	bool ok = true;
	ApiRouter r(ApiRoute{boost::regex{"/foo"}, {}, [&](HR, PM) -> HttpResponsePtr {
		ok = false;
		return std::make_shared<HttpResponse>(204);
	}});
	HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/bar"));
	REQUIRE(resp->status == 404);
	REQUIRE(ok);
}

TEST_CASE("Empty router: no paths matched") {
	ApiRouter r;
	HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/bar"));
	REQUIRE(resp->status == 404);
}
