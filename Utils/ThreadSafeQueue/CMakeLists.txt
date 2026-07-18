cmake_minimum_required(VERSION 3.20)

project(thread_safe_queue
        VERSION 1.0.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCE_FILES
    main.cpp
)

find_package(Threads REQUIRED)

add_executable(thread_safe_queue_exe ${SOURCE_FILES})
target_include_directories(thread_safe_queue_exe PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/includes
)
target_compile_options(thread_safe_queue_exe PRIVATE -O2)
target_link_libraries(thread_safe_queue_exe PRIVATE Threads::Threads)
