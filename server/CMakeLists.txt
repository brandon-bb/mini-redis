cmake_minimum_required(VERSION 3.19)
project(server)

set(CMAKE_CXX_STANDARD 20)
find_package(Boost 1.74.0 REQUIRED)

add_library (
	server.hpp
	server.cpp
	connection.hpp
	connection.cpp
)

add_executable(${PROJECT_NAME} 
	main.cpp
	server.cpp
	connection.cpp
)
