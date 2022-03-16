#pragma once

namespace flint
{
	class Font
	{
		friend class FontManager;
		friend class Renderer;

	public:

		enum Weight
		{
			NORMAL = 0,
			BOLD,
			ITALIC,
		};

	protected:

		Font(void*);
		~Font();

		void* m_pFont;
	};

	class FontManager
	{
	public:

		FontManager();
		~FontManager();

		Font* create(const char* family, unsigned short size, Font::Weight weight = Font::Weight::NORMAL);
		Font* getDefaultFont();

	protected:

		Font* m_pDefaultFont;
	};

};