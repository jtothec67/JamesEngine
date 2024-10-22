#pragma once

#include <SDL2/sdl.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>

namespace JamesEngine
{

	class Window
	{
	public:
		Window(int _width, int _height);
		~Window();

		void Update();

		void ClearWindow();

		void SwapWindows() { SDL_GL_SwapWindow(mWindow); }

		void GetWindowSize(int& _width, int& _height) { _width = mWidth; _height = mHeight; }

		void SetClearColour(glm::vec3 _colour);

		void ActivateVSync(bool _activate) { SDL_GL_SetSwapInterval(_activate ? 1 : 0); mVSync = _activate; }

		bool IsVSyncActive() { return mVSync; }

	private:
		SDL_Window* mWindow;

		int mWidth;
		int mHeight;

		glm::vec3 mClearColour = glm::vec3(0.0f, 0.0f, 0.0f);

		bool mVSync = true;
	};

}