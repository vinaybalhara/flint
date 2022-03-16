#include "Engine.hpp"
#include "JavaScriptInterface.hpp"
#include "Platform.hpp"
#include "Utility.hpp"
#include "Renderer.hpp"
#include <cassert>
#include <map>

namespace flint
{

	Engine* Engine::m_spInstance = nullptr;
	bool Engine::m_bDebug = false;

	Engine::Engine(const EngineParameters& params) : m_params(params),
													 m_pWindow(nullptr),
													 m_pRenderer(nullptr),
													 m_pScriptInterface(nullptr)
	{
	}
	
	Engine::~Engine()
	{
		if (m_pRenderer)
			delete m_pRenderer;
		if (m_pWindow)
			platform::releaseWindow(m_pWindow);
		if (m_pScriptInterface)
			delete m_pScriptInterface;
	}

	Engine* Engine::Create(const EngineParameters& params)
	{
		if (m_spInstance == nullptr)
		{
			m_spInstance = new Engine(params);
			if (params.commandLine && params.commandLine[0] != 0)
			{
				utility::CommandLineArguments options(params.commandLine);
				m_bDebug = options.has("--debug");
				if (m_bDebug)
					platform::createConsole(L"Flint Console");
				if (params.main.empty())
				{
					const std::string option = options.get("--main");
					m_spInstance->m_params.main = std::wstring(option.cbegin(), option.cend());
				}
			}
			if(m_spInstance->m_params.main.empty())
				m_spInstance->m_params.main = L"main.js";
			return m_spInstance;
		}
		return nullptr;
	}
	
	Engine* Engine::Get() { return m_spInstance; }

	void Engine::release()
	{
		delete m_spInstance;
		m_spInstance = nullptr;
		if (m_bDebug)
			platform::releaseConsole();
	}
	
	bool Engine::initialize()
	{
		if (m_pScriptInterface == nullptr)
		{
			m_pScriptInterface = new javascript::Interface(*this);
			m_pRenderer = new Renderer();
			if (m_pScriptInterface->initialize(m_params.main.c_str()))
			{
				m_pWindow = platform::createWindow(this, m_params.size.width, m_params.size.height, m_params.title.c_str());
				if (m_pWindow && platform::createOpenGLContext(m_pWindow))
					return m_pRenderer->createContext(m_params.size);
			}
		}
		return false;
	}

	void Engine::run()
	{
		assert(m_pWindow);
		platform::setState(m_pWindow, (int)m_params.state);
		onLoad();
		platform::startMessagePump(m_pWindow);
		platform::setVisibility(m_pWindow, false);
		onUnload();
		delete m_pRenderer;
		m_pRenderer = nullptr;
		platform::releaseWindow(m_pWindow);
	}

	void Engine::quit()
	{
		platform::quitWindow();
	}

	void Engine::close()
	{
		if(m_pWindow)
			platform::closeWindow(m_pWindow);
	}

	EngineParameters& Engine::getParameters()
	{
		return m_params;
	}

	const Size& Engine::getSize() const
	{
		return m_params.size;
	}

	void Engine::setSize(const Size& size)
	{
		assert(m_pWindow && m_pRenderer->isValid());
		if (m_params.size != size)
			platform::setSize(m_pWindow, size.width, size.height);
	}

	const wchar_t* Engine::getTitle() const
	{
		return m_params.title.c_str();
	}

	void Engine::setTitle(const wchar_t* title)
	{
		assert(m_pWindow);
		m_params.title = title;
		platform::setTitle(m_pWindow, title);
	}

	EngineState Engine::getState() const
	{
		return m_params.state;
	}

	void Engine::setState(EngineState state)
	{
		assert(m_pWindow);
		if (m_params.state != state)
		{
			m_params.state = state;
			platform::setState(m_pWindow, (int)state);
		}
	}

	void Engine::onResize(const Size& size)
	{
		if (m_params.size != size)
		{
			m_params.size = size;
			m_pRenderer->setSize(size);
			if (m_pRenderer->render())
				platform::swapBuffers(m_pWindow);
		}
	}

	void Engine::onUpdate(float delta)
	{
		m_pScriptInterface->update(delta);
		if (m_pRenderer->render())
			platform::swapBuffers(m_pWindow);
		platform::sleep(3);
	}

	void Engine::onLoad()
	{
		m_pScriptInterface->load();
	}

	bool Engine::onBeforeUnload()
	{
		return m_pScriptInterface->beforeUnload();
	}

	void Engine::onUnload()
	{
		m_pScriptInterface->unload();
	}

	bool Engine::onKeyEvent(uint8_t keyCode, const wchar_t* key, uint8_t flags)
	{
		return m_pScriptInterface->event(1 + (flags & 0x1), keyCode, key, flags);
	}

}