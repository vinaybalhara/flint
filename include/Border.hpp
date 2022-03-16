#pragma once

#include "Types.hpp"

namespace flint
{
	class Border
	{
	public:

		static const Border NONE;

		Border(Size::Type w = 0, const Color& c = Color()) : width(w), color(c)
		{
		}

		Size::Type width;
		Color color;
	};

}