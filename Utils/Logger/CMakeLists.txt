cmake_minimum_required(VERSION 3.20)

project(logger 
        VERSION 1.0.0 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #Compiler extensions

find_package(Threads REQUIRED)

add_library(logger_lib STATIC
    clsLogger.cpp
)
target_include_directories(logger_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(logger_lib PRIVATE
    $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    $<$<CONFIG:Release>:-O3>
)
target_link_libraries(logger_lib PUBLIC Threads::Threads)

add_executable(logger_exe main.cpp)
target_include_directories(logger_exe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(logger_exe PRIVATE
    $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    $<$<CONFIG:Release>:-O3>
)
target_link_libraries(logger_exe PRIVATE logger_lib)
