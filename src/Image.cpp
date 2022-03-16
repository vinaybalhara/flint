#include "Image.hpp"
#include "include/core/SkImage.h"

namespace flint
{

	Image::~Image()
	{
		if (m_pImage)
			((SkImage*)m_pImage)->unref();
	}


}