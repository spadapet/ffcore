#pragma once

namespace ff
{
	template<typename T>
	class PointType
	{
	public:
		PointType() = default;
		PointType(const PointType<T>& rhs) = default;
		PointType(T tx, T ty);

		static PointType<T> Zeros();
		static PointType<T> Ones();

		// functions

		void SetPoint(T tx, T ty);
		bool IsNull() const;
		PointType<T> Offset(T tx, T ty) const;
		PointType<T> Offset(const PointType<T>& rhs) const;
		T LengthSquared() const;

		// operators

		PointType<T>& operator=(const PointType<T>& rhs) = default;

		bool operator==(const PointType<T>& rhs) const;
		bool operator!=(const PointType<T>& rhs) const;

		PointType<T> operator+(const PointType<T>& rhs) const;
		PointType<T> operator-(const PointType<T>& rhs) const;
		PointType<T> operator-();

		void operator+=(const PointType<T>& rhs);
		void operator-=(const PointType<T>& rhs);

		void operator*=(T scale);
		void operator*=(const PointType<T>& rhs);
		void operator/=(T scale);
		void operator/=(const PointType<T>& rhs);

		PointType<T> operator*(T scale) const;
		PointType<T> operator*(const PointType<T>& rhs) const;
		PointType<T> operator/(T scale) const;
		PointType<T> operator/(const PointType<T>& rhs) const;

		template<typename T2>
		PointType<T2> ToType() const
		{
			return PointType<T2>((T2)x, (T2)y);
		}

		// data

		union
		{
			struct
			{
				T x;
				T y;
			};

			T arr[2];
		};
	};

	typedef PointType<int> PointInt;
	typedef PointType<short> PointShort;
	typedef PointType<float> PointFloat;
	typedef PointType<double> PointDouble;
	typedef PointType<size_t> PointSize;
	typedef PointType<ff::FixedInt> PointFixedInt;
}

template<typename T>
ff::PointType<T>::PointType(T tx, T ty)
	: x(tx)
	, y(ty)
{
}

template<typename T>
ff::PointType<T> ff::PointType<T>::Zeros()
{
	return PointType<T>((T)0, (T)0);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::Ones()
{
	return PointType<T>((T)1, (T)1);
}

template<typename T>
void ff::PointType<T>::SetPoint(T tx, T ty)
{
	x = tx;
	y = ty;
}

template<typename T>
ff::PointType<T> ff::PointType<T>::Offset(T tx, T ty) const
{
	return PointType<T>(x + tx, y + ty);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::Offset(const PointType<T>& zPt) const
{
	return PointType<T>(x + zPt.x, y + zPt.y);
}

template<typename T>
T ff::PointType<T>::LengthSquared() const
{
	return x * x + y * y;
}

template<typename T>
bool ff::PointType<T>::IsNull() const
{
	return !x && !y;
}

template<typename T>
bool ff::PointType<T>::operator==(const PointType<T>& rhs) const
{
	return x == rhs.x && y == rhs.y;
}

template<typename T>
bool ff::PointType<T>::operator!=(const PointType<T>& rhs) const
{
	return x != rhs.x || y != rhs.y;
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator+(const PointType<T>& rhs) const
{
	return PointType<T>(x + rhs.x, y + rhs.y);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator-(const PointType<T>& rhs) const
{
	return PointType<T>(x - rhs.x, y - rhs.y);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator-()
{
	return PointType<T>(-x, -y);
}

template<typename T>
void ff::PointType<T>::operator+=(const PointType<T>& rhs)
{
	x += rhs.x;
	y += rhs.y;
}

template<typename T>
void ff::PointType<T>::operator-=(const PointType<T>& rhs)
{
	x -= rhs.x;
	y -= rhs.y;
}

template<typename T>
void ff::PointType<T>::operator*=(T scale)
{
	x *= scale;
	y *= scale;
}

template<typename T>
void ff::PointType<T>::operator*=(const PointType<T>& rhs)
{
	x *= rhs.x;
	y *= rhs.y;
}

template<typename T>
void ff::PointType<T>::operator/=(T scale)
{
	x /= scale;
	y /= scale;
}

template<typename T>
void ff::PointType<T>::operator/=(const PointType<T>& rhs)
{
	x /= rhs.x;
	y /= rhs.y;
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator*(T scale) const
{
	return PointType<T>(x * scale, y * scale);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator/(T scale) const
{
	return PointType<T>(x / scale, y / scale);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator*(const PointType<T>& rhs) const
{
	return PointType<T>(x * rhs.x, y * rhs.y);
}

template<typename T>
ff::PointType<T> ff::PointType<T>::operator/(const PointType<T>& rhs) const
{
	return PointType<T>(x / rhs.x, y / rhs.y);
}
