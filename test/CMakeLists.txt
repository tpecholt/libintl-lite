cmake_minimum_required(VERSION 3.26)

project(libintl-test)

add_executable(libintl-test main.cpp)

target_link_libraries(libintl-test libintl)

add_custom_command(TARGET libintl-test POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${CMAKE_CURRENT_SOURCE_DIR}/data/ ${CMAKE_CURRENT_BINARY_DIR}/data/
)