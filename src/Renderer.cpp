#include "Renderer.hpp"
#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrDirectContext.h"
#include "src/gpu/gl/GrGLUtil.h"
#include "include/gpu/gl/GrGLInterface.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"
#include "include/effects/SkImageFilters.h"
//#include "include/effects/SkBlurImageFilter.h"
//#include "include/effects/SkColorMatrixFilter.h"
//#include "include/core/SkMatrix44.h"
#include "include/core/SkRegion.h"
#include "Stage.hpp"
#include "Image.hpp"
#include "RTree.h"

namespace flint
{
	class Paint : public SkPaint {};
	class Canvas : public SkCanvas {};
	class Context : public GrDirectContext {};

	const Border Border::NONE = Border(0);

	static sk_sp<const GrGLInterface> Interface;

	Renderer::Renderer() : m_pContext(nullptr),
						   m_pCanvas(nullptr),
						   m_pPaint(nullptr),
						   m_nFBO(0),
						   m_nTexture(0),
						   m_pStage(new Stage(*this)),
						   m_nStencilBuffer(0),
						   m_redrawRegions(*this),
					       m_bInvalidLayout(true)
	{
	}

	Renderer::~Renderer()
	{
		delete m_pStage;
		if (m_pPaint)
			delete m_pPaint;
		if (m_pCanvas)
			m_pCanvas->getSurface()->unref();
		if (m_pContext)
			m_pContext->unref();
		if (m_nFBO)
		{
			Interface->fFunctions.fDeleteTextures(1, &m_nTexture);
			Interface->fFunctions.fDeleteRenderbuffers(1, &m_nStencilBuffer);
			Interface->fFunctions.fDeleteFramebuffers(1, &m_nFBO);
		}
	}

	bool Renderer::createContext(const Size& size)
	{
		if(!Interface)
			Interface = GrGLMakeNativeInterface();
		bool bResult = false;
		if (m_pCanvas == nullptr && Interface)
		{
			sk_sp<GrDirectContext> context = GrDirectContext::MakeGL(Interface);
			if (context)
			{
				/*const float Vertices[] = { -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
				const float TextureCoords[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

				Interface->fFunctions.fGenVertexArrays(1, &VAO);
				Interface->fFunctions.fBindVertexArray(VAO);
				GrGLuint buffers[2];
				Interface->fFunctions.fGenBuffers(2, buffers);
				Interface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, buffers[0]);
				Interface->fFunctions.fBufferData(GR_GL_ARRAY_BUFFER, 12 * sizeof(float), Vertices, GR_GL_STATIC_DRAW);
				Interface->fFunctions.fEnableVertexAttribArray(0);
				Interface->fFunctions.fVertexAttribPointer(0, 2, GR_GL_FLOAT, GR_GL_FALSE, 0, 0);
				Interface->fFunctions.fBindBuffer(GR_GL_ARRAY_BUFFER, buffers[1]);
				Interface->fFunctions.fBufferData(GR_GL_ARRAY_BUFFER, 12 * sizeof(float), TextureCoords, GR_GL_STATIC_DRAW);
				Interface->fFunctions.fEnableVertexAttribArray(1);
				Interface->fFunctions.fVertexAttribPointer(1, 2, GR_GL_FLOAT, GR_GL_FALSE, 0, 0);
				Interface->fFunctions.fBindVertexArray(0);
				*/
				m_tSize = size;
				const Size::Type width = std::max<Size::Type>(m_tSize.width, 2);
				const Size::Type height = std::max<Size::Type>(m_tSize.height, 2);
				Interface->fFunctions.fGenTextures(1, &m_nTexture);
				Interface->fFunctions.fBindTexture(GR_GL_TEXTURE_2D, m_nTexture);
				Interface->fFunctions.fTexImage2D(GR_GL_TEXTURE_2D, 0, GR_GL_RGBA8, 1920, 1080, 0, GR_GL_RGBA, GR_GL_UNSIGNED_BYTE, NULL);
				Interface->fFunctions.fTexParameteri(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_WRAP_S, GR_GL_CLAMP);
				Interface->fFunctions.fTexParameteri(GR_GL_TEXTURE_2D, GR_GL_TEXTURE_WRAP_T, GR_GL_CLAMP);
			
				Interface->fFunctions.fGenRenderbuffers(1, &m_nStencilBuffer);
				Interface->fFunctions.fBindRenderbuffer(GR_GL_RENDERBUFFER, m_nStencilBuffer);
				Interface->fFunctions.fRenderbufferStorage(GR_GL_RENDERBUFFER, GR_GL_DEPTH24_STENCIL8, 1920, 1080);
				
				Interface->fFunctions.fGenFramebuffers(1, &m_nFBO);
				Interface->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, m_nFBO);
				Interface->fFunctions.fFramebufferTexture2D(GR_GL_FRAMEBUFFER, GR_GL_COLOR_ATTACHMENT0, GR_GL_TEXTURE_2D, m_nTexture, 0);
				Interface->fFunctions.fFramebufferRenderbuffer(GR_GL_FRAMEBUFFER, 0x821A, GR_GL_RENDERBUFFER, m_nStencilBuffer);

				GrGLuint targets[] = { GR_GL_COLOR_ATTACHMENT0 };
				Interface->fFunctions.fDrawBuffers(1, targets);

				GrGLFramebufferInfo info;
				info.fFBOID = (GrGLuint)m_nFBO;
				info.fFormat = GR_GL_RGBA8;
				GrBackendRenderTarget target(width, height, 1, 8, info);
				SkSurfaceProps props;
				sk_sp<SkSurface> surface = SkSurface::MakeFromBackendRenderTarget(context.get(), target, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, &props);
				m_pCanvas = (surface) ? (Canvas*)surface->getCanvas() : NULL;
				if (m_pCanvas)
				{
					surface.release();
					m_pContext = (Context*)context.release();
					m_pPaint = new Paint();
					m_pFont = m_fontManager.getDefaultFont();
					m_pPaint->setAntiAlias(true);
					m_pStage->setSize(m_tSize);
					Interface->fFunctions.fViewport(0, 0, width, height);
					bResult = true;
				}
			}
		}
		return bResult;
	}

	bool Renderer::render()
	{
		const Bound bound(0, 0, m_tSize.width, m_tSize.height);
		SkPath* pPath = nullptr;
		if(m_bInvalidLayout)
			m_pStage->updateLayout(bound, false);
		if (!m_redrawRegions.update(pPath))
			return false;
	
		Interface->fFunctions.fBindFramebuffer(GR_GL_DRAW_FRAMEBUFFER, m_nFBO);
		m_pCanvas->save();
		if (pPath)
			m_pCanvas->clipPath(*pPath);
		m_pStage->render(bound, m_redrawRegions);
		/*IRenderElement* pLast = nullptr;
		for (size_t i = 0; i < m_visibleElems.size(); ++i)
		{
			IRenderElement* pElem = m_visibleElems[i];
			if (pElem == pLast)
				m_pCanvas->save();
			else if (pElem == nullptr)
				m_pCanvas->restore();
			else
			{
				m_pCanvas->setMatrix(pElem->m_transformGlobal);
				if (pElem->m_nOpacity != 255)
					m_pCanvas->saveLayerAlpha(nullptr, pElem->m_nOpacity);
				else
					m_pCanvas->save();
				pElem->draw();
				if(pElem->m_nOverFlow != OverFlow::VISIBLE)
					setClippingRectangle(pElem->m_rect);

				//m_pCanvas->restore();
			}
			pLast = pElem;
		}*/
		m_pCanvas->restore();
		flush();
		Interface->fFunctions.fBindFramebuffer(GR_GL_DRAW_FRAMEBUFFER, 0);
		Interface->fFunctions.fBindFramebuffer(GR_GL_READ_FRAMEBUFFER, m_nFBO);
		Interface->fFunctions.fBlitFramebuffer(0, 0, m_tSize.width, m_tSize.height, 0, 0, m_tSize.width, m_tSize.height, GR_GL_COLOR_BUFFER_BIT, GR_GL_NEAREST);
		return true;
	}

	void Renderer::setSize(const Size& size)
	{
		if (size.width == 0 || size.height == 0)
			m_tSize = size;
		else if (size != m_tSize)
		{
			m_tSize = size;
			Interface->fFunctions.fBindFramebuffer(GR_GL_FRAMEBUFFER, m_nFBO);
			GrGLFramebufferInfo info;
			info.fFBOID = (GrGLuint)m_nFBO;
			info.fFormat = GR_GL_RGBA8;
			GrBackendRenderTarget target(size.width, size.height, 1, 8, info);
			SkSurfaceProps props;
			sk_sp<SkSurface> surface = SkSurface::MakeFromBackendRenderTarget((GrDirectContext*)m_pContext, target, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, NULL, &props);
			if(surface)
			{
				m_pCanvas->getSurface()->unref();
				m_pCanvas = (Canvas*)surface->getCanvas();
				surface.release();
				m_redrawRegions.invalidate();
				Interface->fFunctions.fViewport(0, 0, size.width, size.height);
				m_pStage->setSize(m_tSize);
			}
		}
	}

	void Renderer::setClippingRectangle(const Rectangle& rectangle)
	{
		m_pCanvas->clipRect(SkRect::MakeXYWH((SkScalar)rectangle.position.x, (SkScalar)rectangle.position.y, (SkScalar)rectangle.size.width, (SkScalar)rectangle.size.height), true);
	}

	void Renderer::setExclusionRectangle(const Rectangle& rectangle)
	{
		m_pCanvas->clipRect(SkRect::MakeXYWH((SkScalar)rectangle.position.x, (SkScalar)rectangle.position.y, (SkScalar)rectangle.size.width, (SkScalar)rectangle.size.height), SkClipOp::kDifference, true);
	}

	void Renderer::reset()
	{
		m_pCanvas->resetMatrix();
	}

	void Renderer::save() 
	{
		m_pCanvas->save();
	}
	
	void Renderer::saveAlpha(unsigned char alpha)
	{
		m_pCanvas->saveLayerAlpha(nullptr, alpha);
	}

	void Renderer::restore()
	{
		m_pCanvas->restore();
	}

	unsigned char Renderer::getAlpha() const
	{
		return m_pPaint->getAlpha();
	}

	void Renderer::setAlpha(unsigned char alpha)
	{
		m_pPaint->setAlpha(alpha);
	}

	void Renderer::translate(const Position& position)
	{
		m_pCanvas->translate((SkScalar)position.x, (SkScalar)position.y);
	}

	void Renderer::transform(const SkM44& matrix)
	{
		m_pCanvas->concat(matrix);
	}

	void Renderer::setTransform(const SkM44& matrix)
	{
		m_pCanvas->setMatrix(matrix);
	}

	void Renderer::rotate(float degree, const Position& position)
	{
		m_pCanvas->rotate(degree, (SkScalar)position.x, (SkScalar)position.y);
	}

	void Renderer::invalidate()
	{
		m_redrawRegions.invalidate();
	}

	void Renderer::addRedrawRegion(const Bound& bound)
	{
		m_redrawRegions.add(bound);
	}

	void Renderer::invalidateLayout()
	{
		m_bInvalidLayout = true;
	}

	/*bool Renderer::hitTest(const Position& position, std::list<const IRenderElement*>& out) const
	{
		return false;
	}*/

	void Renderer::deleteLayer(void* pLayer)
	{
		((SkSurface*)pLayer)->unref();
	}

	void* Renderer::createLayer(const Size& size)
	{
		const Size::Type width = size.width + 2;  //for AA
		const Size::Type height = size.height + 2;
		auto info = SkImageInfo::MakeN32Premul(width, height);
		sk_sp<SkSurface> surface = SkSurface::MakeRenderTarget((GrDirectContext*)m_pContext, SkBudgeted::kNo, info);
		return surface.release();
	}

	void* Renderer::setLayer(void* pLayer)
	{
		SkSurface* pSurface = m_pCanvas->getSurface();
		m_pCanvas = (Canvas*)((SkSurface*)pLayer)->getCanvas();
		m_pCanvas->translate(1, 1);
		return pSurface;
	}

	void Renderer::finish()
	{
		m_pCanvas->getSurface()->flush(SkSurface::BackendSurfaceAccess::kPresent, GrFlushInfo());
	}

	void Renderer::drawLayer(void* pLayer)
	{
		static const SkSamplingOptions options(SkFilterMode::kLinear, SkMipmapMode::kNone);
		((SkSurface*)pLayer)->draw(m_pCanvas, -1, -1,  options, m_pPaint);
	}

	void Renderer::setFilter(void* pFilter)
	{
		if (pFilter)
		{
			sk_sp<SkImageFilter> filter((SkImageFilter*)pFilter);
			m_pPaint->setImageFilter(filter);
		}
		else
			m_pPaint->setImageFilter(nullptr);
	}

	Image* Renderer::createImage(const char* filename)
	{
		sk_sp<SkData> data = SkData::MakeFromFileName(filename);
		if (data)
		{
			SkImage* pImage = SkImage::MakeFromEncoded(data).release();
			if (pImage)
				return new Image(pImage, *this);
		}
		return nullptr;
	}

	Font* Renderer::createFont(const char* family, unsigned short size, Font::Weight weight)
	{
		return m_fontManager.create(family, size, weight);
	}

	void Renderer::setFont(Font* pFont)
	{
		if (pFont)
			m_pFont = pFont;
		else
			m_pFont = m_fontManager.getDefaultFont();
	}

	void Renderer::setColor(const Color& color)
	{
		m_pPaint->setARGB(color.alpha, color.red, color.green, color.blue);
	}

	void Renderer::setStrokeWidth(const Size::Type width)
	{
		if (width == 0)
			m_pPaint->setStroke(false);
		else
		{
			m_pPaint->setStrokeWidth((SkScalar)width);
			m_pPaint->setStroke(true);
		}
	}

	void Renderer::draw(const Rectangle& rectangle)
	{
		m_pCanvas->drawRect(SkRect::MakeXYWH((SkScalar)rectangle.position.x, (SkScalar)rectangle.position.y, (SkScalar)rectangle.size.width, (SkScalar)rectangle.size.height), *m_pPaint);
	}

	void Renderer::draw(const Position& p1, const Position& p2)
	{
		m_pCanvas->drawLine(SkPoint::Make(p1.x, p1.y), SkPoint::Make(p2.x, p2.y), *m_pPaint);
	}

	void Renderer::draw(const Position* points, unsigned short count)
	{
		SkPath path;
		path.moveTo(points[0].x, points[0].y);
		for (unsigned short i = 1; i < count; ++i)
			path.lineTo(points[i].x, points[i].y);
		m_pCanvas->drawPath(path, *m_pPaint);
	}

	void Renderer::draw(const Image& image, const Rectangle& rectangle)
	{
		const SkSamplingOptions quality(SkFilterMode::kNearest);
		m_pCanvas->drawImageRect((const SkImage*)image.m_pImage,SkRect::MakeXYWH(0,0,rectangle.size.width, rectangle.size.height), quality, m_pPaint);
	}

	void Renderer::draw(const char* text, const Position& position)
	{
		m_pCanvas->drawString(text, position.x, position.y, *((SkFont*)m_pFont->m_pFont), *m_pPaint);
	}

	void Renderer::flush()
	{
		m_pContext->flush();
	}

}