#pragma once

#include "Types.hpp"
#include "Renderer.hpp"
#include "Border.hpp"
#include "Image.hpp"

#include "include/core/SkM44.h"
#include "include/effects/SkImageFilters.h"
#include "include/effects/SkShaderMaskFilter.h"
#include <vector>

namespace flint
{
	class Renderer;

	struct OverFlow
	{
		enum Enum
		{
			VISIBLE = 0,
			HIDDEN,
			SCROLL,
		};
	};

	class IRenderElement
	{
		friend class Renderer;

		enum Flags
		{
			NONE = 0,
			INVALIDATE_TRANSFORM = 1 << 0,
			INVALIDATE_BOUNDS = 1 << 1,
			INVALIDATE_REGION = 1 << 2,
			INVALIDATE_CACHE = 1 << 3,
			INVALIDATE_ALL = INVALIDATE_TRANSFORM | INVALIDATE_BOUNDS | INVALIDATE_REGION | INVALIDATE_CACHE
		};

	public:

		IRenderElement(Renderer& renderer) : m_renderer(renderer),
											m_pParent(nullptr),
											m_pCache(nullptr),
											m_fRotation(0),
											m_nBorderState(0),
											m_nCacheCounter(0),
											m_nOpacity(255),
											m_bBoundsDirty(false),
											m_backgroundColor(Color(0, 0, 0, 0)),
											m_bTransformsDirty(false),
											m_pBackgroundImage(nullptr),
											m_nOverFlow(OverFlow::HIDDEN),
											m_bDirty(true),
											m_bClipped(true),
											m_nFlags(0),
											m_bVisible(true)
		{
		}

		void add(IRenderElement* pChild)
		{
			if (pChild->m_pParent)
				pChild->m_pParent->remove(pChild);
			pChild->m_pParent	= this;
			pChild->m_bClipped	= m_bClipped;
			m_vecChildren.push_back(pChild);
			pChild->invalidateTransform();
			pChild->invalidateRegion();
		}

		void remove(IRenderElement* pChild)
		{
			auto itr = std::find(m_vecChildren.cbegin(), m_vecChildren.cend(), pChild);
			m_vecChildren.erase(itr);
			pChild->invalidateRegion();
			pChild->m_pParent = nullptr;
			pChild->m_bClipped = true;
		}

		const Size& getSize() const
		{
			return m_rect.size;
		}

		virtual void setSize(const Size& size)
		{
			if (m_rect.size != size)
			{
				m_bClipped = !(m_bVisible && m_pParent);
				m_rect.size = size;
				invalidateRegion();
			}
		}

		const Position& getPosition() const
		{
			return m_rect.position;
		}

		virtual void setPosition(const Position& position)
		{
			if (m_rect.position != position)
			{
				m_transform.setRC(0, 3, position.x);
				m_transform.setRC(1, 3, position.y);
				m_rect.position = position;
				invalidateTransform();
				m_bClipped = !(m_bVisible && m_pParent);
				invalidateRegion();
			}
		}

		void setBackgroundColor(const Color& color)
		{
			m_backgroundColor = color;
			invalidateRegion();
		}

		void setBackgroundImage(Image* pImage)
		{
			if (m_pBackgroundImage != pImage)
			{
				m_pBackgroundImage = pImage;
				invalidate();
			}
		}

		void setXScroll(Position::Type pos)
		{
			m_scrollPos.x = pos;
			invalidate();
			invalidateTransform();
		}

		Position::Type getXScroll() const
		{
			return m_scrollPos.x;
		}

		void invalidateLayout()
		{
			if (!m_bInvalidLayout)
			{
				m_bInvalidLayout = true;
				m_bBoundsDirty = true;
				m_bClipped = false;
				m_renderer.invalidateLayout();
				invalidateRegion();
			}
		}

		virtual void invalidateRegion()
		{
			if(!m_bDirty && !m_bClipped)
			{
				m_bDirty = true;
				m_renderer.addRedrawRegion(m_visibleBounds);
			}
		}

		virtual void invalidateTransform()
		{
			if (!m_bTransformsDirty)
			{
				m_bTransformsDirty = true;
		//		for (size_t i = 0; i < m_vecChildren.size(); ++i)
		//			m_vecChildren[i]->m_bTransformsDirty = true;
			}
		}

		virtual const SkM44& getGlobalTransform()
		{
			if (m_bTransformsDirty)
			{
				if (m_pParent)
				{
					m_transformGlobal = m_pParent->getGlobalTransform();
					m_transformGlobal.preConcat(m_transform);
				}
				else
					m_transformGlobal = m_transform;
				m_bTransformsDirty = false;
			}
			return m_transformGlobal;
		}

		unsigned char getOpacity() const
		{
			return m_nOpacity;
		}

		void setOpacity(unsigned char opacity)
		{
			if (m_nOpacity != opacity)
			{
				m_nOpacity = opacity;
				if (m_pParent && m_bVisible)
					m_pParent->invalidate();
			}
		}

		float getRotation() const
		{
			return m_fRotation;
		}

		virtual void setRotation(float rotation)
		{
			if (m_fRotation != rotation)
			{
				m_fRotation = rotation;
				m_transform.setRotateUnit({ 0,0,1 }, rotation * 0.01745329251994329f);
				//m_transform.preScale(2, 2);
				const float originX = 0.5f * m_rect.size.width;
				const float originY = 0.5f * m_rect.size.height;
				m_transform.preTranslate(-originX, -originY);
				m_transform.postTranslate(m_rect.position.x + originX, m_rect.position.y + originY);
				if (m_pParent && m_bVisible)
					m_pParent->invalidate();
				invalidateTransform();
				invalidateBounds();
				if(!m_bClipped)
					m_renderer.addRedrawRegion(m_globalBound);
			}
		}

		virtual ~IRenderElement()
		{
			if (m_pParent)
				m_pParent->remove(this);
		}

		void release()
		{
			delete this;
		}

		void setBorder(const Border&& border, unsigned char id = 255)
		{
			if (id > 3)
			{
				if (border.width == 0)
					m_nBorderState = 0;
				else
				{
					m_border[0] = std::forward<const Border>(border);
					m_nBorderState = 1;
				}
			}
			else
			{
				m_nBorderState = 2;
				m_border[id] = std::forward<const Border>(border);
			}
		}

		void setBorder(const Border& border, unsigned char id = 255)
		{
			if (id > 3)
			{
				if (border.width == 0)
					m_nBorderState = 0;
				else
				{
					m_border[0] = border;
					m_nBorderState = 1;
				}
			}
			else
			{
				m_nBorderState = 2;
				m_border[id] = border;
			}
		}

		void invalidate()
		{
			if (!m_bDirty)
			{
				m_bDirty = true;
				if (m_pParent && m_bVisible)
					m_pParent->invalidate();
			}
		}

		void hitTest(const Position& point, std::vector<IRenderElement*>& vecElems)
		{
			if (!m_bClipped && m_visibleBounds.isInside(point))
			{
				vecElems.push_back(this);
				for (size_t i = 0; i < m_vecChildren.size(); ++i)
					m_vecChildren[i]->hitTest(point, vecElems);
			}
		}

		virtual void invalidateBounds()
		{
			if (!m_bBoundsDirty && !m_bClipped)
			{
				m_bBoundsDirty = true;
				m_pParent->invalidateBounds();
			}
		}

		virtual const Bound& getLocalBounds()
		{
			m_localBound = Bound(0, 0, m_rect.size.width, m_rect.size.height);
			if (m_nOverFlow == OverFlow::VISIBLE)
			{
				for (size_t i = 0; i < m_vecChildren.size(); ++i)
				{
					Bound bound = m_vecChildren[i]->getLocalBounds();
					const SkM44 transform = m_vecChildren[i]->m_transform;
					const SkV3 xDir = transform * SkV3{ (float)bound.getSize().width, 0.0f, 0.0f };
					const SkV3 yDir = transform * SkV3{ 0.0f, (float)bound.getSize().height, 0.0f };
					const SkV3 pt = xDir + yDir;
					const SkV3 pt0 = { (float)bound.left, (float)bound.top, 0 };
					bound.left = std::min<float>({ pt0.x, xDir.x, yDir.x, pt.x });
					bound.right = std::max<float>({ pt0.x, xDir.x, yDir.x, pt.x });
					bound.top = std::min<float>({ pt0.y, xDir.y, yDir.y, pt.y });
					bound.bottom = std::max<float>({ pt0.y, xDir.y, yDir.y, pt.y });
					bound.translate(Position(transform.rc(0, 3), transform.rc(1, 3)));
					m_localBound.unify(bound);
				}
			}
			return m_localBound;
		}

		virtual const Bound& getGlobalBounds()
		{
			if (m_bBoundsDirty || m_bTransformsDirty)
			{
				m_localBound = getLocalBounds();
				const SkM44& globalTransform = getGlobalTransform();
				const SkV3 xDir = globalTransform * SkV3{ 1.0f, 0.0f, 0.0f };
				const SkV3 yDir = globalTransform * SkV3{ 0.0f, 1.0f, 0.0f };
				const SkV3 pt0 = xDir * m_localBound.left + yDir * m_localBound.top;
				const SkV3 pt1 = xDir * m_localBound.right + yDir * m_localBound.top;
				const SkV3 pt2 = xDir * m_localBound.left + yDir * m_localBound.bottom;
				const SkV3 pt3 = xDir * m_localBound.right + yDir * m_localBound.bottom;

				m_globalBound.left =  std::min<float>({ pt0.x, pt1.x, pt2.x, pt3.x });
				m_globalBound.right = std::max<float>({ pt0.x, pt1.x, pt2.x, pt3.x });
				m_globalBound.top =   std::min<float>({ pt0.y, pt1.y, pt2.y, pt3.y });
				m_globalBound.bottom = std::max<float>({ pt0.y, pt1.y, pt2.y, pt3.y });

				m_globalBound.translate(Position(globalTransform.rc(0, 3), globalTransform.rc(1, 3)));
				m_bBoundsDirty = false;
			}
			return m_globalBound;
		}

		bool getVisibility() const
		{
			return m_bVisible;
		}

		void setVisibility(bool bVisible)
		{
			if (m_bVisible != bVisible)
			{
				m_bVisible = bVisible;
				if (m_pParent)
					m_pParent->invalidate();
			}
		}

		OverFlow::Enum getOverFlow() const
		{
			return m_nOverFlow;
		}

		void setOverFlow(OverFlow::Enum overflow)
		{
			if (m_nOverFlow != overflow)
			{
				m_nOverFlow = overflow;
				invalidate();
			}
		}

	protected:

		void renderBounds()
		{
			//if (!m_bClipped)
			{
				//m_renderer.save();
				//m_renderer.transform(m_transformGlobal);
				//m_renderer.setColor(Color(0, 0, 0));
				//m_renderer.draw(Rectangle(m_localBound.getPosition(), m_localBound.getSize()));
				//m_renderer.restore();
				if(m_bClipped)
					m_renderer.setColor(Color(255, 0, 0));
				else
					m_renderer.setColor(Color(0, 255, 0));
				m_renderer.draw(Rectangle(m_globalBound.getPosition(), m_globalBound.getSize()));
				for (size_t i = 0; i < m_vecChildren.size(); ++i)
					m_vecChildren[i]->renderBounds();
			}
		}

		void updateLayout(const Bound& bound, bool bIgnore)
		{
			m_bClipped = true;
			if (m_bVisible && m_nOpacity != 0)
			{
				getGlobalBounds();
				if (bound.intersect(m_globalBound, m_visibleBounds) && !m_visibleBounds.isEmpty())
				{
					m_bClipped = false;
					if (m_bDirty && !bIgnore)
						m_renderer.addRedrawRegion(m_visibleBounds);
					if (!m_vecChildren.empty())
					{
						for (size_t i = 0; i < m_vecChildren.size(); ++i)
							m_vecChildren[i]->updateLayout(m_visibleBounds, m_bDirty);
					}
				}
			}
		}

		void render(const Bound& bound, const RedrawRegions& region)
		{
			if (!m_bClipped)
			{
				if (m_bDirty || region.isDirty(m_visibleBounds))
				{
					m_bDirty = false;
					m_renderer.save();
					m_renderer.setTransform(m_transformGlobal);
					draw();
					if (!m_vecChildren.empty())
					{
						for (size_t i = 0; i < m_vecChildren.size(); ++i)
							m_vecChildren[i]->render(m_visibleBounds, region);
					}
					m_renderer.restore();
				}
			}
		}

		void draw()
		{
			/*SkImageFilter* filter = SkImageFilters::DropShadow(6, 6, 2, 2, SkColorSetARGB(100, 0, 0, 0), nullptr).release();
			if (this == m_renderer.getRootElement())
			{
				filter->unref();
				filter = nullptr;
			}*/
			const Rectangle rect = { 0, 0, m_rect.size.width, m_rect.size.height };
		
			if (m_backgroundColor.alpha != 0 /*|| filter*/)
			{
				/*m_renderer.setFilter(filter);
				m_renderer.setColor(Color());
				m_renderer.save();
				m_renderer.setExclusionRectangle(Rectangle(0, 0, m_rect.size.width, m_rect.size.height));
				m_renderer.draw(Rectangle(0, 0, m_rect.size.width, m_rect.size.height));
				m_renderer.restore();
				m_renderer.setFilter(nullptr);
				*/
				m_renderer.setColor(m_backgroundColor);
				m_renderer.draw(rect);
			}
			if (m_pBackgroundImage)
			{
				m_renderer.setAlpha(255);
				m_renderer.draw(*m_pBackgroundImage, rect);
			}
			drawBorders();
			if(m_nOverFlow != OverFlow::VISIBLE)
				m_renderer.setClippingRectangle(rect);
			onRender();
		}

		 void onRender()
		{
			/*Font* pFont = m_renderer.createFont("Segoe UI", 12);
			m_renderer.setColor(Color(0, 0, 0));
			m_renderer.setFont(pFont);
			if (m_pCache)
				m_renderer.draw("This is Cached", Position(4, 14));
			else
				m_renderer.draw("This is not Cached", Position(4, 14));
				*/
		};

		void drawBorders()
		{
			const Size::Type width = m_rect.size.width;
			const Size::Type height = m_rect.size.height;
			if (m_nBorderState == 1)
			{
				m_renderer.setStrokeWidth(m_border[0].width);
				m_renderer.setColor(m_border[0].color);
				m_renderer.draw(Rectangle(0, 0, width, height));
				m_renderer.setStrokeWidth(0);
			}
			else if (m_nBorderState == 2)
			{
				Position points[4];
				if (m_border[0].width > 0)
				{
					points[0] = Position(0, 0);
					points[1] = Position(width, 0);
					points[2] = Position(width - m_border[1].width, m_border[0].width);
					points[3] = Position(m_border[3].width, m_border[0].width);
					m_renderer.setColor(m_border[0].color);
					m_renderer.draw(points, 4);
				}
				if (m_border[1].width > 0)
				{
					points[0] = Position(width, 0);
					points[1] = Position(width, height);
					points[2] = Position(width - m_border[1].width, height - m_border[2].width);
					points[3] = Position(width - m_border[1].width, m_border[0].width);
					m_renderer.setColor(m_border[1].color);
					m_renderer.draw(points, 4);
				}
				if (m_border[2].width > 0)
				{
					points[0] = Position(width, height);
					points[1] = Position(0, height);
					points[2] = Position(m_border[3].width, height - m_border[2].width);
					points[3] = Position(width - m_border[1].width, height - m_border[2].width);
					m_renderer.setColor(m_border[2].color);
					m_renderer.draw(points, 4);
				}
				if (m_border[3].width > 0)
				{
					points[0] = Position(0, 0);
					points[1] = Position(0, height);
					points[2] = Position(m_border[3].width, height - m_border[2].width);
					points[3] = Position(m_border[3].width, m_border[0].width);
					m_renderer.setColor(m_border[3].color);
					m_renderer.draw(points, 4);
				}
			}
		}

	protected:

		uint8_t m_nFlags;
		bool m_bTransformsDirty;
		bool m_bBoundsDirty;
		bool m_bClipped;
		bool m_bSimple;
		SkM44 m_transform;
		SkM44 m_transformGlobal;
		Bound m_globalBound;
		std::vector<IRenderElement*> m_vecChildren;
		IRenderElement* m_pParent;
		Image* m_pBackgroundImage;
		Rectangle m_rect;
		Bound m_localBound;
		Bound m_visibleBounds;
		Color m_backgroundColor;
		float m_fRotation;
		unsigned char m_nOpacity;
		Border m_border[4];
		unsigned char m_nBorderState;
		Renderer& m_renderer;
		void* m_pCache;
		bool m_bDirty;
		bool m_bVisible;
		bool m_bInvalidLayout;
		unsigned char m_nCacheCounter;
		OverFlow::Enum m_nOverFlow;
		Position m_scrollPos;
	};

};