cmake_minimum_required(VERSION 2.6)
find_package(wubwubcmake 2)
if(wubwubcmake_DIR)
	include(wubwubcmake/warning_settings)
	add_sane_warning_flags()
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(fetch fetch.cpp)
target_link_libraries(fetch sg--http)
