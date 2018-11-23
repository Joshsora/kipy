cmake_minimum_required(VERSION 3.1)
project(kipy)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(dependencies/libki)
add_subdirectory(dependencies/pybind11)

# Bindings for ki::util
pybind11_add_module(util src/util_bindings.cpp)
target_link_libraries(util PRIVATE ki)

# Bindings for ki::dml
pybind11_add_module(dml src/dml_bindings.cpp)
target_link_libraries(dml PRIVATE ki)

# Bindings for ki::protocol
pybind11_add_module(protocol src/protocol_bindings.cpp)
target_link_libraries(protocol PRIVATE ki)

# Bindings for ki::pclass
pybind11_add_module(pclass src/pclass_bindings.cpp)
target_link_libraries(pclass PRIVATE ki)

# Bindings for ki::serialization
pybind11_add_module(serialization src/serialization_bindings.cpp)
target_link_libraries(serialization PRIVATE ki)
