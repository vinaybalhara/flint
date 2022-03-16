#pragma once

namespace flint
{
	class Renderer;

	class Image
	{
		friend class Renderer;
	
	protected:

		Image(void* pImage, Renderer& renderer) : m_pImage(pImage), m_renderer(renderer) {}
		~Image();


		Renderer& m_renderer;
		void* m_pImage;
	};

}