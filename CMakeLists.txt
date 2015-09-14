cmake_minimum_required(VERSION 2.8.12)

project(sg--http)

set("${CMAKE_PROJECT_NAME}_VERSION_MAJOR" 1)
set("${CMAKE_PROJECT_NAME}_VERSION_MINOR" 0)
set("${CMAKE_PROJECT_NAME}_VERSION_PATCH" 0)

set(headers
	apiserver.hpp
	httpclient.hpp
	httpconnection.hpp
	httpexception.hpp
	http_global.hpp
	httpmessages.hpp
	httpserver.hpp
	http_util.hpp
	socket.hpp
	uri.hpp
)

# Both "SHARED" and the LINKER_LANGUAGE are required because it's header only.
# Shouldn't be necessary anymore in 3.0 where you can have arbitrary targets.
add_library(sg--http SHARED ${headers})
set_target_properties(sg--http PROPERTIES LINKER_LANGUAGE CXX)

find_package(wubwubcmake 1)
if(wubwubcmake_DIR)
	include(wubwubcmake/sane_install)
	sane_install(TARGETS sg--http HEADERS ${headers})
endif()