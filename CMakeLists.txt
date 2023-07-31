cmake_minimum_required (VERSION 3.19)

project (mini-redis)

add_subdirectory (server)
add_executable(${PROJECT_NAME} client.cpp)