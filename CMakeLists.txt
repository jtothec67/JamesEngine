cmake_minimum_required(VERSION ${CMAKE_VERSION})

project(JAMESENGINE)

include_directories(
	src
)

add_library(JamesEngine
	src/JamesEngine/core.cpp src/JamesEngine/core.h
)

add_executable(demo
	src/demo/main.cpp
	src/demo/Employee.cpp src/demo/Employee.h
)

target_link_libraries(demo JamesEngine)