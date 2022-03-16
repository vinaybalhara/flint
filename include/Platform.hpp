#pragma once

#include <string>

namespace flint
{
	class IEngineEventListener;

	namespace platform
	{
		class Window;

		Window* createWindow(IEngineEventListener* pListener, unsigned int width, unsigned int height, const wchar_t* title);
		void releaseWindow(Window*& window);
		bool createOpenGLContext(Window* window);
		void startMessagePump(Window* window);
		void setSize(Window* window, unsigned int width, unsigned int height);
		void setVisibility(Window* window, bool bVisible);
		void setTitle(Window* window, const wchar_t* text);
		void swapBuffers(Window* window);
		void setState(Window* window, int state);
		void closeWindow(Window* window);
		void quitWindow();
		void sleep(int ms);
		void createConsole(const wchar_t* title);
		void releaseConsole();
		std::wstring getCWD();
		std::wstring getFullPath(const wchar_t* path);
		std::wstring getFileName(const wchar_t* path);
	}
}