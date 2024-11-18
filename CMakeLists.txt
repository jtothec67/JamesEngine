cmake_minimum_required(VERSION ${CMAKE_VERSION})

project(JAMESENGINE)

include_directories(
	src
	contrib/include
	contrib/cl-windows-x84_64/include
)

link_directories(
	contrib/cl-windows-x84_64/lib
)

add_library(JamesEngine
	src/JamesEngine/JamesEngine.h

	src/JamesEngine/Core.h
	src/JamesEngine/Core.cpp

	src/JamesEngine/Component.h
	src/JamesEngine/Component.cpp

	src/JamesEngine/Entity.h
	src/JamesEngine/Entity.cpp

	src/JamesEngine/Window.h
	src/JamesEngine/Window.cpp

	src/JamesEngine/TriangleRenderer.h
	src/JamesEngine/TriangleRenderer.cpp

	src/JamesEngine/ModelRenderer.h
	src/JamesEngine/ModelRenderer.cpp

	src/JamesEngine/Transform.h
)

target_link_libraries(JamesEngine Renderer)

add_library(Renderer
	src/Renderer/Font.h
	src/Renderer/Font.cpp

	src/Renderer/Mesh.h
	src/Renderer/Mesh.cpp

	src/Renderer/RenderTexture.h
	src/Renderer/RenderTexture.cpp

	src/Renderer/Shader.h
	src/Renderer/Shader.cpp

	src/Renderer/Texture.h
	src/Renderer/Texture.cpp

	src/Renderer/Model.h
)

target_link_libraries(Renderer SDL2 OpenGL32 glew32 freetype)

add_executable(demo
	src/demo/main.cpp
)

target_link_libraries(demo JamesEngine)