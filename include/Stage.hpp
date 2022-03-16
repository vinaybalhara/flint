#pragma once

#include "RenderElement.hpp"

namespace flint
{

	class Stage : public IRenderElement
	{
	public:

		Stage(Renderer& renderer) : IRenderElement(renderer)
		{
			m_bClipped = false;
			m_backgroundColor = Color(255, 255, 255);
		}

		void setSize(const Size& size)
		{
			if (m_rect.size != size)
			{
				m_localBound = Bound(0, 0, size.width, size.height);
				m_rect.size = size;
				m_globalBound = m_rect;
				m_renderer.invalidateLayout();
				m_renderer.invalidate();
			}
		}

		void setPosition(const Position&) {};
		void setRotation(float rotation) {};
		const SkM44& getGlobalTransform() {	return m_transformGlobal; }
		void invalidateTransform() {}
		void invalidateRegion() { m_renderer.invalidateLayout(); m_renderer.invalidate(); }
		void invalidateBounds() {}
		const Bound& getLocalBounds() { return m_localBound; }
		const Bound& getGlobalBounds() { return m_globalBound; }

	};

}