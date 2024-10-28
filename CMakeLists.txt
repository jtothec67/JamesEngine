cmake_minimum_required(VERSION ${CMAKE_VERSION})

project(JAMESENGINE)

include_directories(
	src
	contrib/include
)

link_directories(
	${PROJECT_SOURCE_DIR}/contrib/lib
)

add_library(JamesEngine
	src/JamesEngine/JamesEngine.h
	src/JamesEngine/Core.cpp
	src/JamesEngine/Core.h
	src/JamesEngine/Component.h
	src/JamesEngine/Component.cpp

	src/JamesEngine/Entity.h
	src/JamesEngine/Entity.cpp

	src/JamesEngine/Window.cpp
	src/JamesEngine/Window.h

	src/JamesEngine/Font.cpp
	src/JamesEngine/Font.h
	src/JamesEngine/Mesh.cpp
	src/JamesEngine/Mesh.h
	src/JamesEngine/RenderTexture.cpp
	src/JamesEngine/RenderTexture.h
	src/JamesEngine/Shader.cpp
	src/JamesEngine/Shader.h
	src/JamesEngine/Model.h
)

target_link_libraries(JamesEngine SDL2 OpenGL32 glew32)

add_executable(demo
	src/demo/main.cpp
)

target_link_libraries(demo JamesEngine)