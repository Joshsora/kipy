cmake_minimum_required(VERSION 3.1)
project(kipy)

add_subdirectory(dependencies/libki)
add_subdirectory(dependencies/pybind11)

# DML Bindings
pybind11_add_module(dml src/dml_bindings.cpp)
target_link_libraries(dml PRIVATE ki)

# Protocol Bindings
pybind11_add_module(protocol src/protocol_bindings.cpp)
target_link_libraries(protocol PRIVATE ki)
