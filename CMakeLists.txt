cmake_minimum_required(VERSION 2.8.12)

project(sg--http)

set("${CMAKE_PROJECT_NAME}_VERSION_MAJOR" 1)
set("${CMAKE_PROJECT_NAME}_VERSION_MINOR" 0)
set("${CMAKE_PROJECT_NAME}_VERSION_PATCH" 0)

find_package(Boost 1.54 COMPONENTS system REQUIRED)

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
	forward.hpp
)

# Both "SHARED" and the LINKER_LANGUAGE are required because it's header only.
# Shouldn't be necessary anymore in 3.0 where you can have arbitrary targets.
add_library(sg--http SHARED ${headers})
target_link_libraries(sg--http PUBLIC ${Boost_LIBRARIES})
set_target_properties(sg--http PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory("tests")

find_package(wubwubcmake 1)
if(wubwubcmake_DIR)
	include(wubwubcmake/sane_install)
	sane_install(TARGETS sg--http HEADERS ${headers})
endif()
