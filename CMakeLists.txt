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
	src/JamesEngine/Transform.cpp

	src/JamesEngine/Resource.h
	src/JamesEngine/Resource.cpp

	src/JamesEngine/Resources.h
	src/JamesEngine/Resources.cpp

	src/JamesEngine/Texture.h
	src/JamesEngine/Texture.cpp

	src/JamesEngine/Model.h

	src/JamesEngine/Mesh.h

	src/JamesEngine/Shader.h

	src/JamesEngine/Font.h

	src/JamesEngine/Input.h
	src/JamesEngine/Input.cpp

	src/JamesEngine/Keyboard.h
	src/JamesEngine/Keyboard.cpp
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