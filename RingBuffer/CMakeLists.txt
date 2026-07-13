cmake_minimum_required(VERSION 3.20)

project(ringbuffer 
        VERSION 1.0.0 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #Compiler extensions

set(SOURCE_FILES 
    main.cpp
)

find_package(Threads REQUIRED)

add_executable(ringbuffer_exe ${SOURCE_FILES})
target_include_directories(ringbuffer_exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(ringbuffer_exe PRIVATE -O2)
target_link_libraries(ringbuffer_exe PRIVATE Threads::Threads)
