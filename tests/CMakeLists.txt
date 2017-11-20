cmake_minimum_required(VERSION 3.0)
find_package(wubwubcmake 2)
if(wubwubcmake_DIR)
	include(wubwubcmake/warning_settings)
	add_sane_warning_flags()
endif()

find_package(Boost COMPONENTS thread system regex REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/..")

add_executable(checker
  selfconnect.cpp
  router.cpp
  streaming.cpp
  check.cpp
  uri.cpp
  timeout.cpp
  abort.cpp
  nocontent.cpp
  reuse.cpp
  premature-close.cpp
)
target_link_libraries(checker ${Boost_LIBRARIES} sg--http)

enable_testing()
add_test(NAME check WORKING_DIRECTORY ${CMAKE_BINARY_DIR} COMMAND checker)

add_custom_target(check
	DEPENDS checker
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	COMMAND tests/checker
)
