#include "Platform.hpp"
#include "EngineEventListener.hpp"
#include <windows.h>
#include <windowsx.h>
#include <io.h>
#include <gl/gl.h>
#include <fcntl.h>
#include <chrono>
#include <iostream>

namespace flint
{
	namespace platform
	{
#define STR_WINDOW_CLASS TEXT("Flint_WindowClass")

		class Window
		{
		public:

			HWND hWnd;
			HDC hDC;
			HGLRC hRC;
		};

		static BYTE KeyboardState[256];
		static wchar_t code[4];
		static const wchar_t* KeyNames[256] = { 0 };
		static IEngineEventListener* EventListener = nullptr;
		static int widthOffset = 0, heightOffset = 0;

		struct _KeyNameMap
		{
			_KeyNameMap()
			{
				KeyNames[VK_CONTROL] = KeyNames[VK_LCONTROL] = KeyNames[VK_RCONTROL] = L"Control";
				KeyNames[VK_SHIFT] = KeyNames[VK_LSHIFT] = KeyNames[VK_RSHIFT] = L"Shift";
				KeyNames[VK_MENU] = KeyNames[VK_LMENU] = KeyNames[VK_RMENU] = L"Alt";
				KeyNames[VK_LWIN] = KeyNames[VK_RWIN] = L"Meta";
				KeyNames[VK_RETURN] = L"Enter";
				KeyNames[VK_SPACE] = L" ";
				KeyNames[VK_CAPITAL] = L"CapsLock";
				KeyNames[VK_SNAPSHOT] = L"PrintScreen";
				KeyNames[VK_TAB] = L"Tab";
				KeyNames[VK_BACK] = L"Backspace";
				KeyNames[VK_DELETE] = L"Delete";
				KeyNames[VK_LEFT] = L"ArrowLeft";
				KeyNames[VK_UP] = L"ArrowUp";
				KeyNames[VK_RIGHT] = L"ArrowRight";
				KeyNames[VK_DOWN] = L"ArrowDown";
				KeyNames[VK_ESCAPE] = L"Escape";
				KeyNames[VK_ACCEPT] = L"Accept";
				KeyNames[VK_HOME] = L"Home";
				KeyNames[VK_END] = L"End";
				KeyNames[VK_F1] = L"F1";
				KeyNames[VK_F2] = L"F2";
				KeyNames[VK_F3] = L"F3";
				KeyNames[VK_F4] = L"F4";
				KeyNames[VK_F5] = L"F5";
				KeyNames[VK_F6] = L"F6";
				KeyNames[VK_F7] = L"F7";
				KeyNames[VK_F8] = L"F8";
				KeyNames[VK_F9] = L"F9";
				KeyNames[VK_F10] = L"F10";
				KeyNames[VK_F11] = L"F11";
				KeyNames[VK_F12] = L"F12";
				KeyNames[VK_PAUSE] = L"Pause";
				KeyNames[VK_INSERT] = L"Insert";
				KeyNames[VK_NUMLOCK] = L"NumLock";
				KeyNames[VK_SCROLL] = L"ScrollLock";
				KeyNames[VK_NEXT] = L"PageDown";
				KeyNames[VK_PRIOR] = L"PageUp";
				KeyNames[VK_APPS] = L"ContextMenu";
				KeyNames[VK_CLEAR] = KeyNames[VK_OEM_CLEAR] = L"Clear";
			}

			const wchar_t* get(uint8_t key) { return KeyNames[key]; }

			uint8_t location(WPARAM key, LPARAM lParam)
			{
				switch (key)
				{
				case VK_SHIFT:
					return (VK_LSHIFT == MapVirtualKey((lParam & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX)) ? 1 : 2;
				case VK_MENU:
				case VK_CONTROL:
					return (lParam & 0x01000000) ? 2 : 1;
				case VK_RWIN:
					return 2;
				case VK_LWIN:
					return 1;
				}
				return 0;
			}

		}KeyNameMap;

		
		LRESULT CALLBACK WndProc(HWND	hWnd, UINT	uMsg, WPARAM	wParam, LPARAM	lParam)
		{
			switch (uMsg)
			{
				case WM_SYSCOMMAND:
				{
					switch (wParam)
					{
					case SC_SCREENSAVE:
					case SC_MONITORPOWER:
						return 0;
					}
					break;
				}
				case WM_SETFOCUS:
					GetKeyboardState(KeyboardState);
					break;
				case WM_ERASEBKGND:
					return 1;
				case WM_CLOSE:
				{
					if (!EventListener->onBeforeUnload())
						PostQuitMessage(0);
					else
						return 0;
				}
				break;
				case WM_WINDOWPOSCHANGED:
				{
					WINDOWPOS* pPos = (WINDOWPOS*)lParam;
					int width = std::max<int>(pPos->cx - widthOffset, 0);
					int height = std::max<int>(pPos->cy - heightOffset, 0);
					EventListener->onResize(Size((Size::Type)width, (Size::Type)height));
					return 0;
				}
				break;
				case WM_SIZE:
				{
					int width = std::max<int>(GET_X_LPARAM(lParam), 0);
					int height = std::max<int>(GET_Y_LPARAM(lParam), 0);
					EventListener->onResize(Size((Size::Type)width, (Size::Type)height));
				}
				break;
				case WM_SYSKEYUP:
				case WM_KEYUP:
				case WM_SYSKEYDOWN:
				case WM_KEYDOWN:
				{
					KeyboardState[wParam] = GetKeyState(wParam);
					const BYTE ctrl = KeyboardState[VK_CONTROL];
					const uint8_t bControl = ctrl >> 7;
					const uint8_t bShift = KeyboardState[VK_SHIFT] >> 7;
					const uint8_t bAlt = KeyboardState[VK_MENU] >> 7;
					const uint8_t bMeta = (KeyboardState[VK_RWIN] | KeyboardState[VK_LWIN]) >> 7;
					const uint8_t bDown = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);
					const uint8_t bRepeat = (lParam >> 30 ) == 0x1;
					uint8_t flags = bDown | bControl << 1 | bShift << 2 | bAlt << 3 | bMeta << 4 | bRepeat << 5;
					const wchar_t* name = KeyNameMap.get(wParam);
					if (name)
						flags |= (KeyNameMap.location(wParam, lParam) << 6);
					else
					{
						KeyboardState[VK_CONTROL] = 0;
						int len = ToUnicode(wParam, (lParam >> 16) & 0xFF, KeyboardState, code, 4, 0);
						KeyboardState[VK_CONTROL] = ctrl;
						if (len == -1)
							len = ToUnicode(wParam, (lParam >> 16) & 0xFF, KeyboardState, code, 4, 0);
						if (len == -1)
							name = L"Dead";
						else if (len == 0)
							name = L"Unidentified";
						else
						{
							code[1] = 0;
							name = code;
						}
					}
					if(!EventListener->onKeyEvent((uint8_t)wParam, name, flags))
						return 0;
				}
				break;
			}
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}

		Window* createWindow(IEngineEventListener* pListener, unsigned int width, unsigned int height, const wchar_t* title)
		{
			EventListener = pListener;
			WNDCLASS	wc = {0};
			wc.lpfnWndProc = (WNDPROC)WndProc;
			wc.hInstance = GetModuleHandle(NULL);
			wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
			wc.hCursor = LoadCursor(NULL, IDC_ARROW);
			wc.style = CS_OWNDC;
			wc.lpszClassName = STR_WINDOW_CLASS;

			if (!RegisterClass(&wc))
				return nullptr;

			DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
			DWORD dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
			
			RECT rc = { 0, 0, (LONG)width, (LONG)height };
			AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
			widthOffset = (rc.right - rc.left) - width;
			heightOffset = (rc.bottom - rc.top) - height;
			HWND hWnd = CreateWindowEx(dwExStyle, STR_WINDOW_CLASS, title, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,  NULL, NULL, wc.hInstance, NULL);
			return (hWnd) ? new Window { hWnd, nullptr, nullptr } : nullptr;
		}

		void releaseWindow(Window*& window)
		{
			if (window->hRC)
			{
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(window->hRC);
				ReleaseDC(window->hWnd, window->hDC);
			}
			DestroyWindow(window->hWnd);
			UnregisterClass(STR_WINDOW_CLASS, GetModuleHandle(NULL));
			delete window;
			window = nullptr;
		}

		bool createOpenGLContext(Window* pWindow)
		{
			PIXELFORMATDESCRIPTOR pfd =
			{
				sizeof(PIXELFORMATDESCRIPTOR),
				1,
				PFD_DRAW_TO_WINDOW |
				PFD_SUPPORT_OPENGL |
				PFD_DOUBLEBUFFER,
				PFD_TYPE_RGBA,
				32,
				0, 0, 0, 0, 0, 0,
				0,
				0,
				0,
				0, 0, 0, 0,
				24,
				0,
				0,
				PFD_MAIN_PLANE,
				0,
				0, 0, 0
			};

			HDC hDC = GetDC(pWindow->hWnd);
			if (hDC == nullptr)
				return false;

			const int format = ChoosePixelFormat(hDC, &pfd);
			if (format == 0 || !SetPixelFormat(hDC, format, &pfd))
			{
				ReleaseDC(pWindow->hWnd, hDC);
				return false;
			}
			HGLRC hRC = wglCreateContext(hDC);
			if (hRC == nullptr)
			{
				ReleaseDC(pWindow->hWnd, hDC);
				return false;
			}
			if (!wglMakeCurrent(hDC, hRC))
			{
				wglDeleteContext(hRC);
				ReleaseDC(pWindow->hWnd, hDC);
				return false;
			}
			pWindow->hDC = hDC;
			pWindow->hRC = hRC;
			return true;
		}

		void startMessagePump(Window* pWindow)
		{
			bool bDone = false;
			MSG msg;
			while (!bDone)
			{
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if (msg.message == WM_QUIT)
						bDone = true;
					else
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
				static auto last = std::chrono::steady_clock::now();
				auto curr = std::chrono::steady_clock::now();
				const std::chrono::duration<float> delta = curr - last;
				last = curr;
				EventListener->onUpdate(delta.count());
			}
		}

		void setVisibility(Window* window, bool bVisible)
		{
			ShowWindow(window->hWnd, (bVisible) ? SW_SHOW : SW_HIDE);
		}

		void setSize(Window* window, unsigned int width, unsigned int height)
		{
			RECT rc = { 0, 0, (LONG)width, (LONG)height };
			SetWindowPos(window->hWnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
		}

		void setTitle(Window* window, const wchar_t* text)
		{
			SetWindowTextW(window->hWnd, text);
		}

		void swapBuffers(Window* window)
		{
			SwapBuffers(window->hDC);
		}

		void setState(Window* window, int state)
		{
			if (state == 2)
				ShowWindow(window->hWnd, SW_MAXIMIZE);
			else if(state == 1)
				ShowWindow(window->hWnd, SW_MINIMIZE);
			else
				ShowWindow(window->hWnd, SW_SHOWNORMAL);
		}

		void closeWindow(Window* window)
		{
			PostMessage(window->hWnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
		}

		void quitWindow()
		{
			PostQuitMessage(0);
		}

		void sleep(int ms)
		{
			Sleep(ms);
		}

		void createConsole(const wchar_t* title)
		{
			FILE* fp = nullptr;
			AllocConsole();
			SetConsoleTitle(title);
			if(GetStdHandle(STD_OUTPUT_HANDLE))
				freopen_s(&fp, "CONOUT$", "w", stdout);
			if(GetStdHandle(STD_ERROR_HANDLE))
				freopen_s(&fp, "CONOUT$", "w", stderr);

			std::ios::sync_with_stdio(true);
			std::wcout.clear();
			std::cout.clear();
			std::wcerr.clear();
			std::cerr.clear();
		}

		void releaseConsole()
		{
			FILE* fp = nullptr;
			freopen_s(&fp, "NUL:", "w", stdout);
			freopen_s(&fp, "NUL:", "w", stderr);
			FreeConsole();
		}

	}

}