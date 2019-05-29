cmake_minimum_required(VERSION 3.10)

cmake_minimum_required(VERSION 3.10)
project(mediautil)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_definitions("-DASSET_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/\"" "-DOUTPUT_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/output/\"")

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(mediautil)

add_subdirectory(test)