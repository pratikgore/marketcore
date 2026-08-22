cmake_minimum_required(VERSION 3.20)

project(biance_adapt 
        VERSION 1.0.0 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #Compiler extensions

set(SOURCE_FILES
    clsRESTBNConnector.cpp
    clsWSBNConnector.cpp
    main.cpp
)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

add_executable(biance_adapt_exe ${SOURCE_FILES})
target_include_directories(biance_adapt_exe PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)
target_compile_options(biance_adapt_exe PRIVATE
    $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    $<$<CONFIG:Release>:-O3>
)

target_link_libraries(biance_adapt_exe PRIVATE
    CURL::libcurl
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
)
# target_link_libraries(biance_adapt_exe PRIVATE logger_lib Threads::Threads)