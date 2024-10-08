cmake_minimum_required(VERSION ${CMAKE_VERSION})

project(JAMESENGINE)

include_directories(
	src
)

add_library(JamesEngine
	src/JamesEngine/JamesEngine.h
	src/JamesEngine/Core.cpp
	src/JamesEngine/Core.h
	src/JamesEngine/Component.h

	src/JamesEngine/Entity.h
)

add_executable(demo
	src/demo/main.cpp
)

target_link_libraries(demo JamesEngine)