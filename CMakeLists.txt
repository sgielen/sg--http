cmake_minimum_required(VERSION 2.8.12)

project(sg--http)

set("${CMAKE_PROJECT_NAME}_VERSION_MAJOR" 1)
set("${CMAKE_PROJECT_NAME}_VERSION_MINOR" 0)
set("${CMAKE_PROJECT_NAME}_VERSION_PATCH" 0)

find_package(wubwubcmake 1)
if(wubwubcmake_DIR)
	include(wubwubcmake/sane_install)
	include(wubwubcmake/warning_settings)
	add_sane_warning_flags()
endif()

find_package(Boost 1.54 COMPONENTS system REQUIRED)
# Boost asio SSL headers include OpenSSL headers
find_package(OpenSSL REQUIRED)

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
if(WIN32)
  set(SG_HTTP_LIBRARIES ${SSL_EAY} ${LIB_EAY} wsock32 ws2_32)
elseif(UNIX)
  set(SG_HTTP_LIBRARIES crypto ssl)
endif()
target_link_libraries(sg--http PUBLIC ${Boost_LIBRARIES} ${SG_HTTP_LIBRARIES})
target_include_directories(sg--http PUBLIC ${OPENSSL_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
set_target_properties(sg--http PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory("tests")

if(wubwubcmake_DIR)
	sane_install(TARGETS sg--http HEADERS ${headers})
endif()
