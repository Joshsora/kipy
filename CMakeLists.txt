project(kipy)
cmake_minimum_required(VERSION 3.1)

add_subdirectory(dependencies/libki)
add_subdirectory(dependencies/pybind11)

pybind11_add_module(dml ki/dml.cpp)
