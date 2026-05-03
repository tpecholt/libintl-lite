 cmake_minimum_required(VERSION 3.10)

project (libintl)

add_library(libintl
   libintl.cpp
)

target_include_directories(libintl
PUBLIC
	../include
)

add_compile_definitions(libintl
PRIVATE
	_CRT_SECURE_NO_WARNINGS
)