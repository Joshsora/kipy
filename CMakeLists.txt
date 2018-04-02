cmake_minimum_required(VERSION 3.1)
project(kipy)

add_subdirectory(dependencies/libki)
add_subdirectory(dependencies/pybind11)

pybind11_add_module(dml src/dml.cpp)
target_link_libraries(dml PRIVATE ki)
