cmake_minimum_required(VERSION 3.20)

project(makert_engine 
        VERSION 1.0.0 
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #Compiler extensions

set(SOURCE_FILES
    clsFeedCommunicator.cpp
    main.cpp
)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

set(SOURCE_FILES
    clsFeedCommunicator.cpp
    clsDataEngine.cpp
    clsOrderBook.cpp
    ${CMAKE_SOURCE_DIR}/BianceFeed/clsWSBNConnector.cpp
    main.cpp
)

add_executable(market_engine_exe ${SOURCE_FILES})
target_include_directories(market_engine_exe PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/BianceFeed/includes
    ${CMAKE_SOURCE_DIR}/third_party
)
target_compile_options(market_engine_exe PRIVATE
    $<$<CONFIG:Debug>:-O0 -g3 -fno-omit-frame-pointer>
    $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    $<$<CONFIG:Release>:-O3>
)
target_compile_definitions(market_engine_exe PRIVATE MC_LOGGING_ENABLED=$<BOOL:${MC_ENABLE_LOGGING}>)

target_link_libraries(market_engine_exe PRIVATE
    CURL::libcurl
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::system
)