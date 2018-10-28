cmake_minimum_required(VERSION 3.1)
project(kipy)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(dependencies/libki)
add_subdirectory(dependencies/pybind11)

# Util Bindings
pybind11_add_module(util src/util_bindings.cpp)
target_link_libraries(util PRIVATE ki)

# DML Bindings
pybind11_add_module(dml src/dml_bindings.cpp)
target_link_libraries(dml PRIVATE ki)

# Protocol Bindings
pybind11_add_module(protocol src/protocol_bindings.cpp)
target_link_libraries(protocol PRIVATE ki)
