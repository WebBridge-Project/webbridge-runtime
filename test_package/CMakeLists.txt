cmake_minimum_required(VERSION 3.26)
project(webbridge_test_package CXX)

find_package(webbridge REQUIRED CONFIG)

add_executable(example src/example.cpp src/Greeter.h)
target_compile_features(example PRIVATE cxx_std_20)
target_link_libraries(example PRIVATE webbridge::webbridge)

webbridge_generate(
	TARGET example
	AUTO
	LANGUAGE cpp
)
