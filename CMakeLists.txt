cmake_minimum_required(VERSION 3.10)
project(karts-backend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(carts-server main.cc)

target_link_libraries(carts-server PRIVATE 
    Threads::Threads
)

if(WIN32)
    target_link_libraries(carts-server PRIVATE ws2_32 crypt32)
endif()