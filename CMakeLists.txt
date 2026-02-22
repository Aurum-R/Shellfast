cmake_minimum_required(VERSION 3.15...3.30)
project(shellfast LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_core
    src/cpp/module.cpp
    src/cpp/filesystem/filesystem.cpp
    src/cpp/text/text.cpp
    src/cpp/system/system.cpp
    src/cpp/process/process.cpp
    src/cpp/network/network.cpp
)

target_include_directories(_core PRIVATE src/cpp)

# Link stdc++fs for older compilers, pthread for threading
target_link_libraries(_core PRIVATE stdc++fs pthread)

install(TARGETS _core DESTINATION shellfast)
