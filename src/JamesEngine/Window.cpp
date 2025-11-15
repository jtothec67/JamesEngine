#include "Window.h"

#include <iostream>
#include <exception>

//#include "imgui.h"
//#include "imgui_impl_sdl2.h"
//#include "imgui_impl_opengl3.h"

namespace JamesEngine
{

	Window::Window(int _width, int _height)
	{
		mWidth = _width;
		mHeight = _height;

		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);

		// Enable 4x MSAA (anti-aliasing)
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

		// Request a modern core context (RenderDoc requires this)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);        // 3.2+ is fine; 4.5 is common
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		mRaw = SDL_CreateWindow("Racing Game 3",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			mWidth, mHeight,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);

		mContext = SDL_GL_CreateContext(mRaw);

		if (!mContext)
		{
			std::cout << "Couldn't create SDL window." << std::endl;
			throw std::exception("Couldn't create SDL window.");
		}

		if (glewInit() != GLEW_OK)
		{
			std::cout << "Couldn't initialise glew." << std::endl;
			throw std::exception("Couldn't initialise glew.");
		}

		// 0 = no vsync, 1 = vsync, 2 = half of vsync
		// (using a value other than 0 on a monitor without gsync/freesync feels very slow)
		SDL_GL_SetSwapInterval(mVSync ? 1 : 0);

		ResetGLModes();

		glClearColor(mClearColour.r, mClearColour.g, mClearColour.b, 1.f);

		size_t availableMemoryKB = 0;
		bool supported = false;

		// Try NVIDIA extension
		if (glewIsExtensionSupported("GL_NVX_gpu_memory_info"))
		{
			GLint availableKB = 0;
			glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &availableKB);
			availableMemoryKB = static_cast<size_t>(availableKB);
			supported = true;
			std::cout << "NVIDIA GPU available memory: " << (availableMemoryKB / 1024) << " MB" << std::endl;
			mVRAMGB = static_cast<float>(availableMemoryKB) / (1024 * 1024);
		}
		// Try AMD extension
		else if (glewIsExtensionSupported("GL_ATI_meminfo"))
		{
			GLint info[4];
			glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, info);
			availableMemoryKB = static_cast<size_t>(info[0]);
			supported = true;
			std::cout << "AMD GPU available memory: " << (availableMemoryKB / 1024) << " MB" << std::endl;
			mVRAMGB = static_cast<float>(availableMemoryKB) / (1024 * 1024);
		}

		// Fallback - use conservative estimate based on max texture size
		if (!supported)
		{
			GLint maxTextureSize = 0;
			glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

			// Conservative estimate - assume at least enough for a few large textures
			// This is very approximate but better than nothing
			size_t estimatedMemoryMB = (maxTextureSize >= 16384) ? 8192 :
				(maxTextureSize >= 8192) ? 4096 :
				(maxTextureSize >= 4096) ? 2048 : 1024;

			availableMemoryKB = estimatedMemoryMB * 1024;
			std::cout << "GPU memory could not be directly queried. Estimated: " << estimatedMemoryMB << " MB" << std::endl;
			mVRAMGB = static_cast<float>(availableMemoryKB) / (1024 * 1024);
		}
	}

	void Window::ResetGLModes()
	{
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
	}

	Window::~Window()
	{
		SDL_GL_DeleteContext(mContext);
		SDL_DestroyWindow(mRaw);
		SDL_Quit();
	}

	void Window::Update()
	{
		SDL_GetWindowSize(mRaw, &mWidth, &mHeight);
		glViewport(0, 0, mWidth, mHeight);
	}

	void Window::ClearWindow()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Window::SetClearColour(glm::vec3 _colour)
	{
		mClearColour = _colour;
		glClearColor(mClearColour.r, mClearColour.g, mClearColour.b, 1.f);
	}

}