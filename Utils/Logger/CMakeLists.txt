cmake_minimum_required(VERSION 3.20)

project(logger 
        VERSION 1.0.0 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #Compiler extensions

set(SOURCE_FILES 
    main.cpp
    clsLogger.cpp
)

find_package(Threads REQUIRED)

add_executable(logger_exe ${SOURCE_FILES})
target_include_directories(logger_exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(logger_exe PRIVATE -O2)
target_link_libraries(logger_exe PRIVATE Threads::Threads)
