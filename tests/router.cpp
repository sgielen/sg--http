#include <apiserver.hpp>
#include <unistd.h>

int main() {
	using namespace sg::http;

	typedef HttpRequestPtr HR;
	typedef std::vector<std::string> PM; // parameters

	std::cout << "1..13" << std::endl;

	{ // Handle a simple request
		bool ok = false;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {"GET"}, [&](HR, PM) -> HttpResponsePtr {
			ok = true;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/foo"));
		if(resp->status == 204) {
			std::cout << "ok 1 Got correct response" << std::endl;
		} else {
			std::cout << "not ok 1 Got correct response" << std::endl;
		}
		if(ok) {
			std::cout << "ok 2 Request was handled" << std::endl;
		} else {
			std::cout << "not ok 2 Request was handled" << std::endl;
		}
	}

	{ // Handle a simple any-method request
		bool ok = false;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {}, [&](HR, PM) -> HttpResponsePtr {
			ok = true;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("FOOBAR", "/foo"));
		if(resp->status == 204) {
			std::cout << "ok 3 Got correct response" << std::endl;
		} else {
			std::cout << "not ok 3 Got correct response" << std::endl;
		}
		if(ok) {
			std::cout << "ok 4 Request was handled" << std::endl;
		} else {
			std::cout << "not ok 4 Request was handled" << std::endl;
		}
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
		if(resp->status == 204) {
			std::cout << "ok 5 Got correct response" << std::endl;
		} else {
			std::cout << "not ok 6 Got correct response" << std::endl;
		}
		if(ok) {
			std::cout << "ok 7 Request was handled" << std::endl;
		} else {
			std::cout << "not ok 8 Request was handled" << std::endl;
		}
	}

	{ // No methods matched
		bool ok = true;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {"GET", "POST"}, [&](HR, PM) -> HttpResponsePtr {
			ok = false;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("FOOBAR", "/foo"));
		if(resp->status == 405) {
			std::cout << "ok 9 Got correct response" << std::endl;
		} else {
			std::cout << "not ok 9 Got correct response" << std::endl;
		}
		if(ok) {
			std::cout << "ok 10 Request was not handled" << std::endl;
		} else {
			std::cout << "not ok 10 Request was not handled" << std::endl;
		}
	}

	{ // No paths matched
		bool ok = true;
		ApiRouter r(ApiRoute{boost::regex{"/foo"}, {}, [&](HR, PM) -> HttpResponsePtr {
			ok = false;
			return std::make_shared<HttpResponse>(204);
		}});
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/bar"));
		if(resp->status == 404) {
			std::cout << "ok 11 Got correct response" << std::endl;
		} else {
			std::cout << "not ok 11 Got correct response" << std::endl;
		}
		if(ok) {
			std::cout << "ok 12 Request was not handled" << std::endl;
		} else {
			std::cout << "not ok 12 Request was not handled" << std::endl;
		}
	}

	{ // Empty router: no paths matched
		ApiRouter r;
		HttpResponsePtr resp = r.handle(std::make_shared<HttpRequest>("GET", "/bar"));
		if(resp->status == 404) {
			std::cout << "ok 13 Got correct response" << std::endl;
		} else {
			std::cout << "not ok 13 Got correct response" << std::endl;
		}
	}
}
