#pragma once

#include "Types.hpp"
#include "EngineEventListener.hpp"
#include <string>

namespace flint
{
	class Renderer;
	namespace platform { class Window; }
	namespace javascript {	class Interface; }

	enum class EngineState:char
	{
		NORMAL = 0,
		MINIMIZED,
		MAXIMIZED
	};

	struct EngineParameters
	{
		EngineParameters() : size(Size(600, 400)),
				 			 title(L"Flint"),
							 commandLine(0),
							 state(EngineState::NORMAL)
		{
		}

		Size size;
		std::wstring title;
		EngineState state;
		char* commandLine;
		std::wstring main;
	};

	class Engine : protected IEngineEventListener
	{
	protected:

		explicit Engine(const EngineParameters& params);
		~Engine();
		Engine() = default;
		Engine(const Engine&) = delete;
		Engine& operator = (const Engine&) = delete;

	public:

		static Engine* Create(const EngineParameters& params = EngineParameters());
		static Engine* Get();
		void release();
		bool initialize();
		void run();
		void quit();
		void close();
		EngineParameters& getParameters();
		const Size& getSize() const;
		void setSize(const Size& size);
		EngineState getState() const;
		void setState(EngineState state);
		const wchar_t* getTitle() const;
		void setTitle(const wchar_t* title);
		Renderer* getRenderer() const { return m_pRenderer; }

	protected:

		void onUpdate(float delta);
		void onLoad();
		bool onBeforeUnload();
		void onUnload();
		void onResize(const Size& size);
		bool onKeyEvent(uint8_t keyCode, const wchar_t* key, uint8_t flags);

	private:

		EngineParameters		m_params;
		Renderer*				m_pRenderer;
		javascript::Interface*	m_pScriptInterface;
		platform::Window*		m_pWindow;
		static Engine*			m_spInstance;
		static bool				m_bDebug;
	};

}