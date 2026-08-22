cmake_minimum_required(VERSION 3.20)

project(marketcore_workspace
        VERSION 1.0.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
${CMAKE_SOURCE_DIR}/BianceFeed/includes
${CMAKE_SOURCE_DIR}/Utils/Logger
${CMAKE_SOURCE_DIR}/Utils/ThreadSafeQueue/includes
${CMAKE_SOURCE_DIR}/Utils/ThreadSafeQueue
${CMAKE_SOURCE_DIR}/Utils/RingBuffer
${CMAKE_SOURCE_DIR}/common
${CMAKE_SOURCE_DIR}/third_party
)

# Keep all binaries/libraries under one top-level build folder.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(BUILD_UTILS "Build Utils subprojects" ON)
option(BUILD_BIANCE_FEED "Build Binance feed executable" ON)

if(BUILD_UTILS)
    add_subdirectory(Utils)
endif()

if(BUILD_BIANCE_FEED)
    add_subdirectory(BianceFeed)
endif()
