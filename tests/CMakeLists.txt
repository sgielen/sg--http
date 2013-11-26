cmake_minimum_required(VERSION 2.6)
add_definitions("-std=c++11")
add_definitions("-Wall -Wextra")

option(USE_LIBCPP "Use libc++ (Mac OS X only)" OFF)
if(USE_LIBCPP)
	add_definitions("-stdlib=libc++")
	set(CMAKE_EXE_LINKER_FLAGS "-stdlib=libc++")
endif()

find_package(Boost COMPONENTS thread system REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")
add_executable(selfconnect selfconnect.cpp)
target_link_libraries(selfconnect ${Boost_LIBRARIES} crypto ssl)
