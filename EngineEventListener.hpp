#pragma once

#include "Types.hpp"

namespace flint
{

	class IEngineEventListener
	{
	public:

		virtual void onUpdate(float delta) = 0;
		virtual void onLoad() = 0;
		virtual bool onBeforeUnload() = 0;
		virtual void onUnload() = 0;
		virtual void onResize(const Size& size) = 0;
		virtual bool onKeyEvent(uint8_t keyCode, const wchar_t* key, uint8_t flags) = 0;
	};

}