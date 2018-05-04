cmake_minimum_required(VERSION 3.9)
project(CS342PR2 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(server server.c shared.h)
add_executable(client client.c shared.h)

target_link_libraries(server rt)
target_link_libraries(client rt)