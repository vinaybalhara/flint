#pragma once

#include <cstdint>

namespace flint
{
	class Engine;

	namespace javascript
	{
		class Environment;

		class Interface
		{
		public:

			Interface(Engine& engine);
			~Interface();

			bool initialize(const char* main);
			void load();
			bool update(float delta);
			bool beforeUnload();
			void unload();
			bool event(uint8_t type, uint8_t, const wchar_t*, uint8_t);

		protected:

			Engine& m_engine;
			Environment* m_pEnvironment;
		};

	}
}