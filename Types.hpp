#pragma once

#include <cstdint>

namespace flint
{

	struct Color
	{
		explicit Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) : red(r), blue(b), green(g), alpha(a) {}
		explicit Color(uint32_t color) : blue(color & 0xFF), green((color >> 8) & 0xFF), red((color >> 16) & 0xFF), alpha(color >> 24) {}
		bool operator == (const Color& c) const { return c.red == red && c.green == green && c.blue == blue && c.alpha == alpha; }
		bool operator != (const Color& c) const { return c.red != red || c.green != green || c.blue != blue || c.alpha != alpha; }
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t alpha;
	};

	template <typename T>
	struct SizeT
	{
		typedef T Type;
		SizeT(const T& w = 0, const T& h = 0) : width(w), height(h) {}
		bool operator == (const SizeT& s) const { return s.width == width && s.height == height; }
		bool operator != (const SizeT& s) const { return s.width != width || s.height != height; }
		SizeT operator + (const SizeT& s) const { return SizeT(width + s.width, height + s.height); }
		SizeT operator - (const SizeT& s) const { return SizeT(width - s.width, height - s.height); }
		T width;
		T height;
	};
	using Size = SizeT<uint16_t>;

	template <typename T>
	struct PositionT
	{
		typedef T Type;
		PositionT(const T& a = 0, const T& b = 0) : x(a), y(b) {}
		bool operator == (const PositionT& p) const { return p.x == x && p.y == y; }
		bool operator != (const PositionT& p) const { return p.x != x || p.y != y; }
		PositionT operator + (const PositionT& p) const { return PositionT(x + p.x, y + p.y); }
		PositionT& operator += (const PositionT& p) { x += p.x; y += p.y; return *this; }
		PositionT operator - (const PositionT& p) const { return PositionT(x - p.x, y - p.y); }
		PositionT operator - () const { return PositionT(-x, -y); }
		T x;
		T y;
	};
	using Position = PositionT<int32_t>;

	struct Rectangle
	{
		Rectangle(const Position& pos = Position(), const Size& sz = Size()) : position(pos), size(sz) {}
		Rectangle(const Position::Type& x, const Position::Type& y, const Size::Type& width, const Size::Type& height) : position(Position(x, y)), size(Size(width, height)) {}
		Size size;
		Position position;

		bool intersection(const Rectangle& rect, Rectangle& rectOut) const
		{
			if (rect.position.x + rect.size.width <= position.x ||
				rect.position.x >= position.x + size.width ||
				rect.position.y + rect.size.height <= position.y ||
				rect.position.y >= position.y + size.height)
				return false;

			rectOut.position.x = (position.x > rect.position.x) ? position.x : rect.position.x;
			rectOut.position.y = (position.y > rect.position.y) ? position.y : rect.position.y;
			rectOut.size.width = ((position.x + size.width < rect.position.x + rect.size.width) ? position.x + size.width : rect.position.x + rect.size.width) - rectOut.position.x;
			rectOut.size.height = ((position.y + size.height < rect.position.y + rect.size.height) ? position.y + size.height : rect.position.y + rect.size.height) - rectOut.position.y;
			return true;
		}

		bool inside(const Position& point) const
		{
			return !(point.x < position.x || point.x > position.x + size.width || point.y < position.y || point.y > position.y + size.height);
		}

		bool outside(const Rectangle& rect) const
		{
			return ((rect.position.x + rect.size.width) < position.x ||
				rect.position.x > (size.width + position.x) ||
				(rect.position.y + rect.size.height) < position.y ||
				rect.position.y > (size.height + position.y));
		}

		void offset(const Size::Type offset)
		{
			position.x += offset;
			position.y += offset;
			size.width -= 2 * offset;
			size.height -= 2 * offset;
		}

	};

	struct Bound
	{
		Position::Type left;
		Position::Type top;
		Position::Type right;
		Position::Type bottom;

		Bound(Position::Type l=0, Position::Type t=0, Position::Type r=0, Position::Type b=0) : left(l), top(t), right(r), bottom(b) {}
		Bound(const Rectangle& rc) : left(rc.position.x), top(rc.position.y), right(rc.position.x + rc.size.width), bottom(rc.size.height)	{}

		Size getSize() const { return Size(right - left, bottom - top); }
		Position getPosition() const { return Position(left, top); }
		void unify(const Bound& bound)
		{
			left = (bound.left < left) ? bound.left : left;
			top = (bound.top < top) ? bound.top : top;
			right = (bound.right > right) ? bound.right : right;
			bottom = (bound.bottom > bottom) ? bound.bottom : bottom;
		}

		void translate(const Position& offset)
		{
			left += offset.x;
			right += offset.x;
			top += offset.y;
			bottom += offset.y;
		}

		bool isEmpty() const 
		{
			return (left == right) || (top == bottom);
		}

		bool isInside(const Position& point) const
		{
			return (point.x >= left && point.x <= right && point.y >= top && point.y <= bottom);
		}

		bool intersect(const Bound& other)
		{
			return intersect(other, *this);
		}

		bool intersect(const Bound& other, Bound& out) const
		{
			if (right <= other.left || left >= other.right || bottom <= other.top || top >= other.bottom)
				return false;
			out.left = (left > other.left) ? left : other.left;
			out.right = (right < other.right) ? right : other.right;
			out.top = (top > other.top) ? top : other.top;
			out.bottom = (bottom < other.bottom) ? bottom : other.bottom;
			return true;
		}

	};

}
