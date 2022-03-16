#pragma once

#include "FontManager.hpp"
#include "include/core/SkTypeface.h"
#include "include/core/SkFont.h"

#include <map>
#include <string>
#include <sstream>

namespace flint
{

	Font::Font(void* pFont) : m_pFont(pFont)
	{
	}

	Font::~Font() 
	{
		delete ((SkFont*)m_pFont);
	}

	struct string_no_case_comparator
	{
		struct compare
		{
			bool operator() (const unsigned char& c1, const unsigned char& c2) const 
			{
				return tolower(c1) < tolower(c2);
			}
		};

		bool operator() (const std::string& s1, const std::string& s2) const
		{
			return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), compare());
		}
	};

	static std::map<std::string, Font*, string_no_case_comparator> g_mapFonts;
	static unsigned int g_nFontManagers = 0;

	FontManager::FontManager(): m_pDefaultFont(nullptr)
	{
		++g_nFontManagers;
	}

	FontManager::~FontManager()
	{
		if (--g_nFontManagers == 0)
		{
			for (auto itr = g_mapFonts.begin(); itr != g_mapFonts.end(); ++itr)
				delete itr->second;
			g_mapFonts.clear();
		}
	}

	Font* FontManager::getDefaultFont() 
	{
		if (m_pDefaultFont == nullptr)
			m_pDefaultFont = create("Arial", 12, Font::Weight::NORMAL);
		return m_pDefaultFont;
	}

	Font* FontManager::create(const char* family, unsigned short size, Font::Weight weight)
	{
		std::ostringstream key;
		key << family << ";" << size << ";" << weight;
		std::string k = key.str();
		Font* pFont = nullptr;
		auto itr = g_mapFonts.find(k);
		if (itr != g_mapFonts.end())
			pFont = itr->second;
		else
		{
			sk_sp<SkTypeface> face = SkTypeface::MakeFromName(family, SkFontStyle::Normal());
			if(face)
			{
				SkFont* pSkFont = new SkFont(face, (SkScalar)size);
				pSkFont->setHinting(SkFontHinting::kFull);
				pSkFont->setSubpixel(true);
				pSkFont->setEdging(SkFont::Edging::kSubpixelAntiAlias);
				pFont = new Font(pSkFont);
				g_mapFonts.insert(std::make_pair(std::move(k), pFont));
			}
		}
		return pFont;
	}

}