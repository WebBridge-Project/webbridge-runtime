cmake_minimum_required(VERSION 3.26)
project(webbridge_test_package CXX)

find_package(webbridge REQUIRED CONFIG)

add_executable(example src/example.cpp src/Greeter.h)
target_compile_features(example PRIVATE cxx_std_20)
target_link_libraries(example PRIVATE webbridge::webbridge)

# Deliberately not calling webbridge_generate() here: it needs a Python venv
# with jinja2/tree-sitter/tree-sitter-cpp, pip-installed on first use, which
# needs network access. ConanCenter's CI builds (and tests) this package
# with network disabled, so test_package only exercises the plain C++
# property/event API directly - see the comment in src/example.cpp.
# webbridge_generate() itself is still fully functional for real consumers
# with Python installed; it's just not exercised by this automated test.
