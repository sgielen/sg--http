cmake_minimum_required(VERSION 2.6)
find_package(wubwubcmake 1)
if(wubwubcmake_DIR)
	include(wubwubcmake/warning_settings)
	add_sane_warning_flags()
endif()
add_definitions("-std=c++11")

option(USE_LIBCPP "Use libc++ (Mac OS X only)" OFF)
if(USE_LIBCPP)
	add_definitions("-stdlib=libc++")
	set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++")
endif()

find_package(Boost COMPONENTS thread system regex REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(checker selfconnect.cpp router.cpp streaming.cpp check.cpp uri.cpp)
target_link_libraries(checker ${Boost_LIBRARIES} sg--http)

enable_testing()
add_test(NAME check WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND checker)

add_custom_target(check
	DEPENDS checker
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND tests/checker
)
