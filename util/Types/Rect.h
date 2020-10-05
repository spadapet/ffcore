#pragma once

namespace ff
{
	template<typename T>
	class RectType
	{
	public:
		RectType() = default;
		RectType(T tLeft, T tTop, T tRight, T tBottom);
		RectType(const RectType<T>& rhs) = default;
		RectType(const PointType<T>& rhs1, const PointType<T>& rhs2);
		explicit RectType(const PointType<T>& rhs);

		static RectType<T> Zeros();

		T Width() const;
		T Height() const;
		T Area() const;
		T CenterX() const;
		T CenterY() const;
		PointType<T> Size() const;
		PointType<T> Center() const;
		PointType<T> TopLeft() const;
		PointType<T> TopRight() const;
		PointType<T> BottomLeft() const;
		PointType<T> BottomRight() const;
		RectType<T> LeftEdge() const;
		RectType<T> TopEdge() const;
		RectType<T> RightEdge() const;
		RectType<T> BottomEdge() const;
		std::array<PointType<T>, 4> Corners() const;

		bool IsEmpty() const;
		bool IsZeros() const;
		bool PointInRect(const PointType<T>& point) const;
		bool PointTouchesRect(const PointType<T>& point) const;
		bool DoesTouch(const RectType<T>& rhs) const; // returns true if (*this) touches rhs at all
		bool DoesTouchHorizIntersectVert(const RectType<T>& rhs) const;
		bool DoesTouchVertIntersectHoriz(const RectType<T>& rhs) const;
		bool DoesIntersect(const RectType<T>& rhs) const;
		bool IsInside(const RectType<T>& rhs) const; // returns true if (*this) is totally inside of rhs
		bool IsOutside(const RectType<T>& rhs) const; // returns true if (*this) is totally outside of rhs

		void SetRect(T tLeft, T tTop, T tRight, T tBottom);
		void SetRect(const PointType<T>& topLeft, const PointType<T>& bottomRight);
		void SetRect(T tRight, T tBottom);
		void SetRect(const PointType<T>& bottomRight);

		RectType<T> Intersect(const RectType<T>& rhs) const;
		RectType<T> Bound(const RectType<T>& rhs) const;
		RectType<T> Interpolate(const RectType<T>& rhs, double value) const; // 0 is lhs , 1 is rhs
		RectType<T> CenterWithin(const RectType<T>& rhs) const; // center (*this) within rhs, if bigger than rhs, center on top of rhs
		RectType<T> CenterOn(const PointType<T>& point) const; // center (*this) on top of point
		RectType<T> ScaleToFit(const RectType<T>& rhs) const; // moves bottom-right so that (*this) would fit within rhs, but keeps aspect ratio
		RectType<T> MoveInside(const RectType<T>& rhs) const; // move this rect so that it is inside of rhs (impossible to do if rhs is smaller than *this)
		RectType<T> MoveOutside(const RectType<T>& rhs) const; // move this rect in one direction so that it is outside of rhs
		RectType<T> MoveTopLeft(const PointType<T>& point) const; // keeps the same size, but moves the rect to point
		RectType<T> MoveTopLeft(T tx, T ty) const; // same as above
		RectType<T> Crop(const RectType<T>& rhs) const; // cut off my edges so that I'm inside of rhs

		RectType<T> Normalize() const;
		RectType<T> Offset(T tx, T ty) const;
		RectType<T> Offset(const PointType<T>& point) const;
		RectType<T> OffsetSize(T tx, T ty) const;
		RectType<T> OffsetSize(const PointType<T>& point) const;
		RectType<T> Deflate(T tx, T ty) const;
		RectType<T> Deflate(T tx, T ty, T tx2, T ty2) const;
		RectType<T> Deflate(const PointType<T>& point) const;
		RectType<T> Deflate(const RectType<T>& rhs) const;
		RectType<T> Inflate(T tx, T ty) const;
		RectType<T> Inflate(T tx, T ty, T tx2, T ty2) const;
		RectType<T> Inflate(const PointType<T>& point) const;
		RectType<T> Inflate(const RectType<T>& rhs) const;

		// operators

		RectType<T>& operator=(const RectType<T>& rhs) = default;
		bool operator==(const RectType<T>& rhs) const;
		bool operator!=(const RectType<T>& rhs) const;

		void operator+=(const PointType<T>& point);
		void operator-=(const PointType<T>& point);

		RectType<T> operator+(const PointType<T>& point) const;
		RectType<T> operator-(const PointType<T>& point) const;

		RectType<T> operator*(const RectType<T>& rhs) const;
		RectType<T> operator*(const PointType<T>& rhs) const;
		RectType<T> operator*(T rhs) const;
		RectType<T> operator/(const RectType<T>& rhs) const;
		RectType<T> operator/(const PointType<T>& rhs) const;
		RectType<T> operator/(T rhs) const;

		template<typename T2>
		RectType<T2> ToType() const
		{
			return RectType<T2>((T2)left, (T2)top, (T2)right, (T2)bottom);
		}

		union
		{
			struct
			{
				T left;
				T top;
				T right;
				T bottom;
			};

			T arr[4];
		};
	};

	typedef RectType<int> RectInt;
	typedef RectType<short> RectShort;
	typedef RectType<float> RectFloat;
	typedef RectType<double> RectDouble;
	typedef RectType<size_t> RectSize;
	typedef RectType<ff::FixedInt> RectFixedInt;
}

template<typename T>
ff::RectType<T>::RectType(T tLeft, T tTop, T tRight, T tBottom)
	: left(tLeft)
	, top(tTop)
	, right(tRight)
	, bottom(tBottom)
{
}

template<typename T>
ff::RectType<T>::RectType(const PointType<T>& rhs1, const PointType<T>& rhs2)
	: left(rhs1.x)
	, top(rhs1.y)
	, right(rhs2.x)
	, bottom(rhs2.y)
{
}

template<typename T>
ff::RectType<T>::RectType(const PointType<T>& rhs)
	: left((T)0)
	, top((T)0)
	, right(rhs.x)
	, bottom(rhs.y)
{
}

// static
template<typename T>
ff::RectType<T> ff::RectType<T>::Zeros()
{
	return RectType<T>((T)0, (T)0, (T)0, (T)0);
}

template<typename T>
T ff::RectType<T>::Width() const
{
	return right - left;
}

template<typename T>
T ff::RectType<T>::Height() const
{
	return bottom - top;
}

template<typename T>
T ff::RectType<T>::Area() const
{
	return Width() * Height();
}

template<typename T>
T ff::RectType<T>::CenterX() const
{
	return (left + right) / 2;
}

template<typename T>
T ff::RectType<T>::CenterY() const
{
	return (top + bottom) / 2;
}

template<typename T>
ff::PointType<T> ff::RectType<T>::Size() const
{
	return PointType<T>(right - left, bottom - top);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::Center() const
{
	return PointType<T>(CenterX(), CenterY());
}

template<typename T>
bool ff::RectType<T>::IsEmpty() const
{
	return bottom == top && right == left;
}

template<typename T>
bool ff::RectType<T>::IsZeros() const
{
	return !left && !top && !right && !bottom;
}

template<typename T>
bool ff::RectType<T>::PointInRect(const PointType<T>& point) const
{
	return point.x >= left && point.x < right && point.y >= top && point.y < bottom;
}

template<typename T>
bool ff::RectType<T>::PointTouchesRect(const PointType<T>& point) const
{
	return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
}

template<typename T>
ff::PointType<T> ff::RectType<T>::TopLeft() const
{
	return PointType<T>(left, top);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::TopRight() const
{
	return PointType<T>(right, top);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::BottomLeft() const
{
	return PointType<T>(left, bottom);
}

template<typename T>
ff::PointType<T> ff::RectType<T>::BottomRight() const
{
	return PointType<T>(right, bottom);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::LeftEdge() const
{
	return RectType<T>(left, top, left, bottom);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::TopEdge() const
{
	return RectType<T>(left, top, right, top);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::RightEdge() const
{
	return RectType<T>(right, top, right, bottom);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::BottomEdge() const
{
	return RectType<T>(left, bottom, right, bottom);
}

template<typename T>
std::array<ff::PointType<T>, 4> ff::RectType<T>::Corners() const
{
	std::array<ff::PointType<T>, 4> corners =
	{
		TopLeft(),
		TopRight(),
		BottomRight(),
		BottomLeft(),
	};

	return corners;
}

template<typename T>
void ff::RectType<T>::SetRect(T tLeft, T tTop, T tRight, T tBottom)
{
	left = tLeft;
	top = tTop;
	right = tRight;
	bottom = tBottom;
}

template<typename T>
void ff::RectType<T>::SetRect(const PointType<T>& topLeft, const PointType<T>& bottomRight)
{
	left = topLeft.x;
	top = topLeft.y;
	right = bottomRight.x;
	bottom = bottomRight.y;
}

template<typename T>
void ff::RectType<T>::SetRect(T tRight, T tBottom)
{
	left = (T)0;
	top = (T)0;
	right = tRight;
	bottom = tBottom;
}

template<typename T>
void ff::RectType<T>::SetRect(const PointType<T>& bottomRight)
{
	left = (T)0;
	top = (T)0;
	right = bottomRight.x;
	bottom = bottomRight.y;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Offset(T tx, T ty) const
{
	return ff::RectType<T>(left + tx, top + ty, right + tx, bottom + ty);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Offset(const PointType<T>& point) const
{
	return ff::RectType<T>(left + point.x, top + point.y, right + point.x, bottom + point.y);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::OffsetSize(T tx, T ty) const
{
	return ff::RectType<T>(left, top, right + tx, bottom + ty);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::OffsetSize(const PointType<T>& point) const
{
	return ff::RectType<T>(left, top, right + point.x, bottom + point.y);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Normalize() const
{
	RectType<T> rect = *this;

	if (rect.left > rect.right)
	{
		std::swap(rect.left, rect.right);
	}

	if (top > bottom)
	{
		std::swap(rect.top, rect.bottom);
	}

	return rect;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Deflate(T tx, T ty) const
{
	return RectType<T>(left + tx, top + ty, right - tx, bottom - ty).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Deflate(T tx, T ty, T tx2, T ty2) const
{
	return RectType<T>(left + tx, top + ty, right - tx2, bottom - ty2).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Deflate(const PointType<T>& point) const
{
	return RectType<T>(left + point.x, top + point.y, right - point.x, bottom - point.y).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Deflate(const RectType<T>& rhs) const
{
	return RectType<T>(left + rhs.left, top + rhs.top, right - rhs.right, bottom - rhs.bottom).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Inflate(T tx, T ty) const
{
	return RectType<T>(left - tx, top - ty, right + tx, bottom + ty).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Inflate(T tx, T ty, T tx2, T ty2) const
{
	return RectType<T>(left - tx, top - ty, right + tx2, bottom + ty2).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Inflate(const PointType<T>& point) const
{
	return RectType<T>(left - point.x, top - point.y, right + point.x, bottom + point.y).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Inflate(const RectType<T>& rhs) const
{
	return RectType<T>(left - rhs.left, top - rhs.top, right + rhs.right, bottom + rhs.bottom).Normalize();
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Intersect(const RectType<T>& rhs) const
{
	RectType<T> rect(
		std::max(left, rhs.left),
		std::max(top, rhs.top),
		std::min(right, rhs.right),
		std::min(bottom, rhs.bottom));

	if (rect.left > rect.right || rect.top > rect.bottom)
	{
		return RectType<T>::Zeros();
	}

	return rect;
}

template<typename T>
bool ff::RectType<T>::DoesTouch(const RectType<T>& rhs) const
{
	return right >= rhs.left && left <= rhs.right && bottom >= rhs.top && top <= rhs.bottom;
}

template<typename T>
bool ff::RectType<T>::DoesTouchHorizIntersectVert(const RectType<T>& rhs) const
{
	return right >= rhs.left && left <= rhs.right && bottom > rhs.top && top < rhs.bottom;
}

template<typename T>
bool ff::RectType<T>::DoesTouchVertIntersectHoriz(const RectType<T>& rhs) const
{
	return right > rhs.left && left < rhs.right && bottom >= rhs.top && top <= rhs.bottom;
}

template<typename T>
bool ff::RectType<T>::DoesIntersect(const RectType<T>& rhs) const
{
	return right > rhs.left && left < rhs.right && bottom > rhs.top && top < rhs.bottom;
}

template<typename T>
bool ff::RectType<T>::IsInside(const RectType<T>& rhs) const
{
	return left >= rhs.left && right <= rhs.right && top >= rhs.top && bottom <= rhs.bottom;
}

template<typename T>
bool ff::RectType<T>::IsOutside(const RectType<T>& rhs) const
{
	return left >= rhs.right || right <= rhs.left || top >= rhs.bottom || bottom <= rhs.top;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Bound(const RectType<T>& rhs) const
{
	return RectType<T>(
		std::min(left, rhs.left),
		std::min(top, rhs.top),
		std::max(right, rhs.right),
		std::max(bottom, rhs.bottom));
}

template<typename T>
ff::RectType<T> ff::RectType<T>::CenterWithin(const RectType<T>& rhs) const
{
	T left2 = rhs.left + (rhs.Width() - Width()) / 2;
	T top2 = rhs.top + (rhs.Height() - Height()) / 2;

	return RectType<T>(left2, top2, left2 + Width(), top2 + Height());
}

template<typename T>
ff::RectType<T> ff::RectType<T>::CenterOn(const PointType<T>& point) const
{
	T left2 = point.x - Width() / 2;
	T top2 = point.y - Height() / 2;

	return RectType<T>(left2, top2, left2 + Width(), top2 + Height());
}

template<typename T>
ff::RectType<T> ff::RectType<T>::MoveInside(const RectType<T>& rhs) const
{
	RectType<T> rect = *this;

	if (rect.left < rhs.left)
	{
		T offset = rhs.left - rect.left;

		rect.left += offset;
		rect.right += offset;
	}
	else if (rect.right > rhs.right)
	{
		T offset = rect.right - rhs.right;

		rect.left -= offset;
		rect.right -= offset;
	}

	if (rect.top < rhs.top)
	{
		T offset = rhs.top - rect.top;

		rect.top += offset;
		rect.bottom += offset;
	}
	else if (rect.bottom > rhs.bottom)
	{
		T offset = rect.bottom - rhs.bottom;

		rect.top -= offset;
		rect.bottom -= offset;
	}

	return rect;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::MoveOutside(const RectType<T>& rhs) const
{
	RectType<T> rect = *this;

	if (DoesIntersect(rhs))
	{
		T leftMove = right - rhs.left;
		T rightMove = rhs.right - left;
		T topMove = bottom - rhs.top;
		T bottomMove = rhs.bottom - top;
		T move = std::min({ leftMove, rightMove, topMove, bottomMove });

		if (move == leftMove)
		{
			rect = rect.Offset(-leftMove, (T)0);
		}

		if (move == topMove)
		{
			rect = rect.Offset((T)0, -topMove);
		}

		if (move == rightMove)
		{
			rect = rect.Offset(rightMove, (T)0);
		}

		if (move == bottomMove)
		{
			rect = rect.Offset((T)0, bottomMove);
		}
	}

	return rect;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::MoveTopLeft(const PointType<T>& point) const
{
	Offset(point.x - left, point.y - top);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::MoveTopLeft(T tx, T ty) const
{
	Offset(tx - left, ty - top);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Crop(const RectType<T>& rhs) const
{
	ff::RectType<T> rect = *this;

	if (rect.left < rhs.left)
	{
		rect.left = rhs.left;
	}

	if (rect.right > rhs.right)
	{
		rect.right = rhs.right;
	}

	if (rect.top < rhs.top)
	{
		rect.top = rhs.top;
	}

	if (rect.bottom > rhs.bottom)
	{
		rect.bottom = rhs.bottom;
	}

	return rect;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::ScaleToFit(const RectType<T>& rhs) const
{
	RectType<T> rect = *this;

	if (rect.left == rect.right)
	{
		if (rect.top != rect.bottom)
		{
			rect.bottom = rect.top + rhs.Height();
		}
	}
	else if (rect.top == rect.bottom)
	{
		rect.right = rect.left + rhs.Width();
	}
	else
	{
		double ratio = (double)rect.Width() / (double)rect.Height();

		if ((double)rhs.Width() / ratio > (double)rhs.Height())
		{
			rect.right = rect.left + (T)((double)rhs.Height() * ratio);
			rect.bottom = rect.top + rhs.Height();
		}
		else
		{
			rect.right = rect.left + rhs.Width();
			rect.bottom = rect.top + (T)((double)rhs.Width() / ratio);
		}
	}

	return rect;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::Interpolate(const RectType<T>& rhs, double value) const
{
	RectDouble dr1((double)left, (double)top, (double)right, (double)bottom);
	RectDouble dr2((double)rhs.left, (double)rhs.top, (double)rhs.right, (double)rhs.bottom);

	return RectType<T>(
		(T)((dr2.left - dr1.left) * value + dr1.left),
		(T)((dr2.top - dr1.top) * value + dr1.top),
		(T)((dr2.right - dr1.right) * value + dr1.right),
		(T)((dr2.bottom - dr1.bottom) * value + dr1.bottom));
}

template<typename T>
bool ff::RectType<T>::operator==(const RectType<T>& rhs) const
{
	return std::memcmp(this, &rhs, sizeof(rhs)) == 0;
}

template<typename T>
bool ff::RectType<T>::operator!=(const RectType<T>& rhs) const
{
	return !(*this == rhs);
}

template<typename T>
void ff::RectType<T>::operator+=(const PointType<T>& point)
{
	left += point.x;
	top += point.y;
	right += point.x;
	bottom += point.y;
}

template<typename T>
void ff::RectType<T>::operator-=(const PointType<T>& point)
{
	left -= point.x;
	top -= point.y;
	right -= point.x;
	bottom -= point.y;
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator+(const PointType<T>& point) const
{
	return RectType<T>(left + point.x, top + point.y, right + point.x, bottom + point.y);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator-(const PointType<T>& point) const
{
	return RectType<T>(left - point.x, top - point.y, right - point.x, bottom - point.y);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator*(const RectType<T>& rhs) const
{
	return RectType<T>(left * rhs.left, top * rhs.top, right * rhs.right, bottom * rhs.bottom);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator*(const PointType<T>& rhs) const
{
	return RectType<T>(left * rhs.x, top * rhs.y, right * rhs.x, bottom * rhs.y);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator*(T rhs) const
{
	return RectType<T>(left * rhs, top * rhs, right * rhs, bottom * rhs);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator/(const RectType<T>& rhs) const
{
	return RectType<T>(left / rhs.left, top / rhs.top, right / rhs.right, bottom / rhs.bottom);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator/(const PointType<T>& rhs) const
{
	return RectType<T>(left / rhs.x, top / rhs.y, right / rhs.x, bottom / rhs.y);
}

template<typename T>
ff::RectType<T> ff::RectType<T>::operator/(T rhs) const
{
	return RectType<T>(left / rhs, top / rhs, right / rhs, bottom / rhs);
}
