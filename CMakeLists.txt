cmake_minimum_required(VERSION 3.1)

project(network_rpc C CXX)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")

add_subdirectory(network)
add_subdirectory(proto_rpc)
add_subdirectory(examples)


