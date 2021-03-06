cmake_minimum_required(VERSION 3.0)

project(sg--http)

set("${CMAKE_PROJECT_NAME}_VERSION_MAJOR" 1)
set("${CMAKE_PROJECT_NAME}_VERSION_MINOR" 3)
set("${CMAKE_PROJECT_NAME}_VERSION_PATCH" 0)

find_package(wubwubcmake 2)
if(wubwubcmake_DIR)
	include(wubwubcmake/sane_install)
	include(wubwubcmake/warning_settings)
	add_sane_warning_flags()
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost 1.54 COMPONENTS system thread regex REQUIRED)

option(SGHTTP_USE_SSL "Whether or not to enable SSL support" ON)
if(SGHTTP_USE_SSL)
	find_package(OpenSSL REQUIRED)
endif()

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

add_library(sg--http INTERFACE)

if(WIN32)
	target_link_libraries(sg--http INTERFACE wsock32 ws2_32)
endif()

if(SGHTTP_USE_SSL)
	target_compile_definitions(sg--http INTERFACE SG_HTTP_SSL)
	target_include_directories(sg--http INTERFACE ${OPENSSL_INCLUDE_DIR})
	if(WIN32)
		target_link_libraries(sg--http INTERFACE ${SSL_EAY} ${LIB_EAY})
	elseif(UNIX)
		# Pthread is a workaround for when boost-thread doesn't link it
		# Encountered it on some machine.
		target_link_libraries(sg--http INTERFACE crypto ssl pthread)
	endif()
endif()

target_link_libraries(sg--http INTERFACE ${Boost_LIBRARIES})
target_include_directories(sg--http INTERFACE ${Boost_INCLUDE_DIRS})

add_subdirectory("tests")
add_subdirectory("examples")

if(wubwubcmake_DIR)
	sane_install(EXPORT TARGETS sg--http HEADERS ${headers})
endif()
