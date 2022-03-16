#pragma once

#include "Types.hpp"
#include "FontManager.hpp"
#include "Renderer.hpp"
#include "RedrawRegions.hpp"
#include <string>
#include <vector>

class SkM44;

namespace flint
{
	class IRenderElement;
	class Stage;
	class Image;
	class Font;
	class Paint;
	class Context;
	class Canvas;

	class Renderer
	{
	public:

		Renderer();
		~Renderer();
		Renderer(const Renderer&) = delete;
		Renderer& operator = (const Renderer&) = delete;

		bool createContext(const Size& size);
		const Size& getSize() const { return m_tSize; }
		Canvas* getCanvas() const { return m_pCanvas; }
		void setSize(const Size& size);
		void setClippingRectangle(const Rectangle& rectangle);
		void setExclusionRectangle(const Rectangle& rectangle);
		void translate(const Position& position);
		void transform(const SkM44& transform);
		void setTransform(const SkM44& transform);
		void rotate(float degree, const Position& position);
		void invalidate();
		void addRedrawRegion(const Bound& bound);
		void invalidateLayout();
		void reset();
		void save();
		void saveAlpha(unsigned char alpha);
		void restore();
		unsigned char getAlpha() const;
		void setAlpha(unsigned char alpha);
		Stage* getStage() const { return m_pStage; }
		Font* createFont(const char* family, unsigned short size, Font::Weight weight = Font::Weight::NORMAL);
		Image* createImage(const char* filename);
		void setFilter(void* pFilter);
		void deleteLayer(void* pLayer);
		void* createLayer(const Size& size);
		void* setLayer(void* layer);
		bool render();
		void finish();
		void drawLayer(void* pLayer);
		void setFont(Font* font);
		void setColor(const Color& color);
		void setStrokeWidth(const Size::Type width);
		void draw(const Rectangle& rectangle);
		void draw(const Position& p1, const Position& p2);
		void draw(const Position* points, unsigned short count);
		void draw(const Image& image, const Rectangle& rectangle);
		void draw(const char* text, const Position& position);
		void flush();
		
	private:

		RedrawRegions					m_redrawRegions;
		Size							m_tSize;
		Stage*							m_pStage;
		Paint*							m_pPaint;
		Canvas*							m_pCanvas;
		Font*							m_pFont;
		Context*						m_pContext;
		FontManager						m_fontManager;
		unsigned int					m_nFBO;
		unsigned int					m_nTexture;
		unsigned int					m_nStencilBuffer;
		bool							m_bInvalidLayout;
	};

}