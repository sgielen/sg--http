#pragma once

#include <string>
#include <sstream>
#include <stdexcept>
#include <map>

namespace sg { namespace http {

struct Uri {
	std::string scheme; // excluding ://
	std::string username; // optional
	std::string password; // optional
	std::string hostname;
	std::string port; // optional
	std::string path; // optional, including first slash
	std::string query; // optional, excluding question mark
	std::string location; // optional, excluding '#'

	static bool default_port(const std::string &scheme, const std::string &port) {
		if(scheme == "http") {
			return port == "http" || port == "80";
		} else if(scheme == "https") {
			return port == "https" || port == "443";
		}
		return false;
	}

	std::string toPathString() const {
		std::stringstream ss;
		ss << path;
		if(query.length() > 0) {
			ss << "?" << query;
		}
		if(location.length() > 0) {
			ss << "#" << location;
		}
		return ss.str();
	}

	std::string toString() const {
		std::stringstream ss;
		ss << scheme << "://";
		if(username.length() > 0) {
			// TODO: urlencode username/password?
			ss << username;
			if(password.length() > 0) {
				ss << ":" << password;
			}
			ss << "@";
		}
		ss << hostname;
		if(port.length() > 0 && !default_port(scheme, port)) {
			ss << ":" << port;
		}
		ss << toPathString();
		return ss.str();
	}

	// <scheme>://[username[:password]@]hostname[:port][/path...][?query...][#location...]
	Uri(std::string uri) {
		size_t readpos = 0;
		size_t schemepos = uri.find("://");
		if(schemepos == std::string::npos) {
			throw std::runtime_error("Invalid uri, no scheme: " + uri);
		}
		scheme = uri.substr(0, schemepos);
		readpos = schemepos + 3;
		size_t slashpos = uri.find('/', readpos);
		size_t authpos  = uri.find('@', readpos);
		if(slashpos != std::string::npos && authpos > slashpos) {
			authpos = std::string::npos; // this @ does not belong to the auth part
		}
		if(authpos != std::string::npos) {
			std::string auth = uri.substr(readpos, authpos - readpos);
			size_t colon = auth.find(':');
			if(colon == std::string::npos) {
				username = auth;
			} else {
				username = auth.substr(0, colon);
				password = auth.substr(colon + 1);
			}
			readpos = authpos + 1;
		}
		size_t portpos = uri.find(':', readpos);
		if(slashpos != std::string::npos && portpos > slashpos) {
			portpos = std::string::npos; // this colon does not start the port
		}
		size_t querypos = uri.find('?', readpos);
		size_t locpos = uri.find('#', readpos);
		if(locpos != std::string::npos && querypos > locpos) {
			querypos = std::string::npos; // this question mark does not start the query
		}
		if((querypos != std::string::npos && slashpos > querypos)
		|| (locpos   != std::string::npos && slashpos > locpos)) {
			slashpos = std::string::npos; // this slash does not start the path
		}
		size_t endhost = slashpos != std::string::npos ? slashpos :
				querypos != std::string::npos ? querypos :
				locpos != std::string::npos ? locpos : uri.length();
		if(portpos != std::string::npos && (slashpos == std::string::npos || slashpos >= portpos)) {
			hostname = uri.substr(readpos, portpos - readpos);
			port = uri.substr(portpos + 1, endhost - portpos - 1);
		} else {
			hostname = uri.substr(readpos, endhost - readpos);
		}
		readpos = endhost;
		size_t endpath = querypos != std::string::npos ? querypos :
				locpos != std::string::npos ? locpos : uri.length();
		if(slashpos != std::string::npos && endpath > slashpos) {
			path = uri.substr(readpos, endpath - readpos);
			readpos = endpath;
		} else {
			path = "/"; // default path
		}
		size_t endquery = locpos != std::string::npos ? locpos : uri.length();
		if(querypos != std::string::npos && endquery > querypos) {
			query = uri.substr(readpos + 1, endquery - readpos - 1);
			readpos = endquery;
		}
		if(locpos != std::string::npos) {
			location = uri.substr(readpos + 1);
		}
	}

	std::map<std::string, std::string> queryParameters() {
		std::map<std::string, std::string> res;
		bool in_key = true;
		std::string key;
		std::string str;
		for(size_t i = 0; i < query.size(); ++i) {
			if(in_key && query[i] == '=') {
				in_key = false;
			} else if(query[i] == '&') {
				in_key = true;
				res[key] = str;
				str.clear();
				key.clear();
			} else if(in_key) {
				key += query[i];
			} else {
				str += query[i];
			}
		}
		if(!in_key) {
			res[key] = str;
		}
		return res;
	}
};

}}
