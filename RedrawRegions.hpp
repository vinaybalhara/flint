#pragma once

#include "Renderer.hpp"
#include "RTree.h"
#include "include/core/SkPath.h"
#include "Types.hpp"
#include <vector>

namespace flint
{

	class RedrawRegions
	{
		using RTree2D = RTree<char, int, 2, float, 8, 1>;
		static const unsigned int MINIMUM_AREA = 150 * 150;

	public:

		RedrawRegions(Renderer& r) : m_renderer(r),
									 m_nState(2)
		{
		}

		~RedrawRegions()
		{
		}

		void add(const Bound& rc)
		{
			if (m_nState != 2)
			{
				const int min[2] = { rc.left, rc.top };
				const int max[2] = { rc.right, rc.bottom };
				Tree.Insert(min, max, 0);
				m_nState = 1;
			}
		}

		void invalidate()
		{
			if (m_nState != 2)
			{
				m_nState = 2;
				Tree.RemoveAll();
			}
		}

		bool isDirty(const Bound& b) const
		{
			if (m_nState == 2)
				return true;
			for (unsigned int i = 0; i < m_vecRegions.size(); ++i)
			{
				if(m_vecRegions[i].intersects(SkRect::MakeLTRB(b.left, b.top, b.right, b.bottom)))
					return true;
			}
			return false;
		}

		bool update(SkPath*& pPath)
		{
			if (m_nState == 0)
				return false;
			else if (m_nState == 2)
				pPath = nullptr;
			else
			{
				m_vecRegions.clear();
				m_path.rewind();
			
				RTree2D::Node* pNode = nullptr;
				Tree.GetRoot(pNode);
				assert(pNode && pNode->m_count > 0);
				for (int i = 0; i < pNode->m_count; ++i)
				{
					RTree2D::Branch& b = pNode->m_branch[i];
					SkRect rc = SkRect::MakeLTRB(b.m_rect.m_min[0], b.m_rect.m_min[1], b.m_rect.m_max[0], b.m_rect.m_max[1]);
					m_vecRegions.push_back(std::move(rc));
				}
				Tree.RemoveAll();
				for (unsigned int i = 0; i < m_vecRegions.size();)
				{
					SkRect& my = m_vecRegions[i];
					const unsigned int myArea = my.width() * my.height();
					bool bMerged = false;
					for (int j = 0; j != i && j < m_vecRegions.size(); ++j)
					{
						SkRect& ot = m_vecRegions[j];
						SkRect merged = SkRect::MakeLTRB(std::min(my.fLeft, ot.fLeft), std::min(my.fTop, ot.fTop), std::max(my.fRight, ot.fRight), std::max(my.fBottom, ot.fBottom));
						const unsigned int area = merged.width() * merged.height();
						const unsigned int tArea = ot.width() * ot.height();
						if (area <= (MINIMUM_AREA + myArea + tArea))
						{
							m_vecRegions.push_back(std::move(merged));
							m_vecRegions.erase(m_vecRegions.cbegin() + i);
							m_vecRegions.erase(m_vecRegions.cbegin() + j);
							bMerged = true;
							break;
						}
					}
					if (m_vecRegions.size() == 1)
						break;
					if (!bMerged)
						++i;
					else
						i = 0;
				}
				for (unsigned int i = 0; i < m_vecRegions.size(); ++i)
				{
					m_vecRegions[i].outset(2, 2);
					m_path.addRect(m_vecRegions[i]);
				}
				pPath = &m_path;
			}
			m_nState = 0;
			return true;
		}

	protected:

		unsigned char m_nState;
		std::vector<SkRect> m_vecRegions;
		RTree2D Tree;
		Renderer& m_renderer;
		SkPath m_path;
	};

}