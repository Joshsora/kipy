cmake_minimum_required(VERSION 3.1)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(KI_BUILD_EXAMPLES OFF CACHE BOOL "Determines whether to build examples." FORCE)
set(KI_BUILD_TESTS OFF CACHE BOOL "Determines whether to build tests." FORCE)

set(ZLIB_STATIC ON)

add_subdirectory(dependencies/libki)
add_subdirectory(dependencies/pybind11)

set(PYTHON_SOURCES
	src/lib.cpp
	src/util.cpp
	src/dml.cpp
	src/protocol.cpp
	src/pclass.cpp
	src/serialization.cpp
)
pybind11_add_module(libki_python ${PYTHON_SOURCES})
target_link_libraries(libki_python PRIVATE ki)
set_target_properties(libki_python PROPERTIES OUTPUT_NAME lib)
