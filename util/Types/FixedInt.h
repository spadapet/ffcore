#pragma once

namespace ff
{
	// 24 bits of whole number and 8 bits of fraction
	class FixedInt
	{
	public:
		inline FixedInt() = default;
		inline FixedInt(const FixedInt& rhs) = default;
		inline FixedInt(int data);
		inline FixedInt(float data);
		inline FixedInt(double data);
		inline FixedInt(bool data);
		inline FixedInt(unsigned int data);
		inline FixedInt(unsigned long long data);
		inline FixedInt(long double data);

		inline operator int() const;
		inline operator float() const;
		inline operator double() const;
		inline operator bool() const;

		inline FixedInt& operator=(const FixedInt& rhs) = default;

		inline bool operator!() const;
		inline bool operator==(const FixedInt& rhs) const;
		inline bool operator!=(const FixedInt& rhs) const;
		inline bool operator<(const FixedInt& rhs) const;
		inline bool operator>(const FixedInt& rhs) const;
		inline bool operator<=(const FixedInt& rhs) const;
		inline bool operator>=(const FixedInt& rhs) const;

		inline FixedInt operator-() const;
		inline FixedInt operator+() const;
		inline FixedInt operator++();
		inline FixedInt operator++(int after);
		inline FixedInt operator--();
		inline FixedInt operator--(int after);

		inline FixedInt& operator+=(const FixedInt& rhs);
		inline FixedInt& operator-=(const FixedInt& rhs);
		inline FixedInt& operator*=(const FixedInt& rhs);
		inline FixedInt& operator/=(const FixedInt& rhs);

		inline FixedInt operator+(const FixedInt& rhs) const;
		inline FixedInt operator-(const FixedInt& rhs) const;
		inline FixedInt operator*(const FixedInt& rhs) const;
		inline FixedInt operator/(const FixedInt& rhs) const;
		inline FixedInt operator%(const FixedInt& rhs) const;

		inline FixedInt& operator*=(int rhs);
		inline FixedInt& operator/=(int rhs);
		inline FixedInt operator*(int rhs) const;
		inline FixedInt operator/(int rhs) const;

		inline FixedInt abs() const;
		inline FixedInt ceil() const;
		inline FixedInt floor() const;
		inline FixedInt trunc() const;
		inline FixedInt copysign(FixedInt sign) const;
		inline void swap(FixedInt& rhs);

		inline static FixedInt FromRaw(int data);
		inline int GetRaw() const;

	private:
		inline static FixedInt FromExpanded(int64_t data);
		inline int64_t GetExpanded() const;

		static const int FIXED_COUNT = 8;
		static const int FIXED_MAX = (1 << FIXED_COUNT);
		static const int LO_MASK = FIXED_MAX - 1;
		static const int HI_MASK = ~LO_MASK;

		int _data;
	};
}

ff::FixedInt::FixedInt(int data)
	: _data(data << FIXED_COUNT)
{
}

ff::FixedInt::FixedInt(float data)
	: _data((int)(data* FIXED_MAX))
{
}

ff::FixedInt::FixedInt(double data)
	: _data((int)(data* FIXED_MAX))
{
}

ff::FixedInt::FixedInt(bool data)
	: _data((int)data << FIXED_COUNT)
{
}

ff::FixedInt::FixedInt(unsigned int data)
	: _data((int)(data << FIXED_COUNT))
{
}

ff::FixedInt::FixedInt(unsigned long long data)
	: _data((int)(data << FIXED_COUNT))
{
}

ff::FixedInt::FixedInt(long double data)
	: _data((int)(data* FIXED_MAX))
{
}

ff::FixedInt::operator int() const
{
	return _data >> FIXED_COUNT;
}

ff::FixedInt::operator float() const
{
	return (float)(double)*this;
}

ff::FixedInt::operator double() const
{
	return (double)_data / FIXED_MAX;
}

ff::FixedInt::operator bool() const
{
	return _data != 0;
}

bool ff::FixedInt::operator!() const
{
	return !_data;
}

bool ff::FixedInt::operator==(const ff::FixedInt& rhs) const
{
	return _data == rhs._data;
}

bool ff::FixedInt::operator!=(const ff::FixedInt& rhs) const
{
	return _data != rhs._data;
}

bool ff::FixedInt::operator<(const ff::FixedInt& rhs) const
{
	return _data < rhs._data;
}

bool ff::FixedInt::operator>(const ff::FixedInt& rhs) const
{
	return _data > rhs._data;
}

bool ff::FixedInt::operator<=(const ff::FixedInt& rhs) const
{
	return _data <= rhs._data;
}

bool ff::FixedInt::operator>=(const ff::FixedInt& rhs) const
{
	return _data >= rhs._data;
}

ff::FixedInt ff::FixedInt::operator-() const
{
	return FromRaw(-_data);
}

ff::FixedInt ff::FixedInt::operator+() const
{
	return FromRaw(+_data);
}

ff::FixedInt ff::FixedInt::operator++()
{
	return *this = (*this + ff::FixedInt(1));
}

ff::FixedInt ff::FixedInt::operator++(int after)
{
	ff::FixedInt orig = *this;
	*this = (*this + ff::FixedInt(1));
	return orig;
}

ff::FixedInt ff::FixedInt::operator--()
{
	return *this = (*this - ff::FixedInt(1));
}

ff::FixedInt ff::FixedInt::operator--(int after)
{
	ff::FixedInt orig = *this;
	*this = (*this - ff::FixedInt(1));
	return orig;
}

ff::FixedInt& ff::FixedInt::operator+=(const ff::FixedInt& rhs)
{
	return *this = (*this + rhs);
}

ff::FixedInt& ff::FixedInt::operator-=(const ff::FixedInt& rhs)
{
	return *this = (*this - rhs);
}

ff::FixedInt& ff::FixedInt::operator*=(const ff::FixedInt& rhs)
{
	return *this = (*this * rhs);
}

ff::FixedInt& ff::FixedInt::operator/=(const ff::FixedInt& rhs)
{
	return *this = (*this / rhs);
}

ff::FixedInt ff::FixedInt::operator+(const ff::FixedInt& rhs) const
{
	return FromRaw(_data + rhs._data);
}

ff::FixedInt ff::FixedInt::operator-(const ff::FixedInt& rhs) const
{
	return FromRaw(_data - rhs._data);
}

ff::FixedInt ff::FixedInt::operator*(const ff::FixedInt& rhs) const
{
	return FromExpanded((GetExpanded() * rhs.GetExpanded()) >> FIXED_COUNT);
}

ff::FixedInt ff::FixedInt::operator/(const ff::FixedInt& rhs) const
{
	return FromExpanded((GetExpanded() << FIXED_COUNT) / rhs.GetExpanded());
}

ff::FixedInt ff::FixedInt::operator%(const ff::FixedInt& rhs) const
{
	return FromRaw(_data % rhs._data);
}

ff::FixedInt& ff::FixedInt::operator*=(int rhs)
{
	return *this = (*this * rhs);
}

ff::FixedInt& ff::FixedInt::operator/=(int rhs)
{
	return *this = (*this / rhs);
}

ff::FixedInt ff::FixedInt::operator*(int rhs) const
{
	return FromRaw(_data * rhs);
}

ff::FixedInt ff::FixedInt::operator/(int rhs) const
{
	return FromRaw(_data / rhs);
}

ff::FixedInt ff::FixedInt::abs() const
{
	return FromRaw(std::abs(_data));
}

ff::FixedInt ff::FixedInt::ceil() const
{
	return FromRaw((_data & HI_MASK) + ((_data & LO_MASK) != 0) * FIXED_MAX);
}

ff::FixedInt ff::FixedInt::floor() const
{
	return FromRaw(_data & HI_MASK);
}

ff::FixedInt ff::FixedInt::trunc() const
{
	return FromRaw((_data & HI_MASK) + (_data < 0) * ((_data & LO_MASK) != 0) * FIXED_MAX);
}

ff::FixedInt ff::FixedInt::copysign(ff::FixedInt sign) const
{
	return sign._data >= 0 ? abs() : -abs();
}

void ff::FixedInt::swap(ff::FixedInt& rhs)
{
	std::swap(_data, rhs._data);
}

ff::FixedInt ff::FixedInt::FromRaw(int data)
{
	ff::FixedInt value;
	value._data = data;
	return value;
}

int ff::FixedInt::GetRaw() const
{
	return _data;
}

ff::FixedInt ff::FixedInt::FromExpanded(int64_t data)
{
	ff::FixedInt value;
	value._data = (int)data;
	return value;
}

int64_t ff::FixedInt::GetExpanded() const
{
	return (int64_t)_data;
}

inline ff::FixedInt operator "" _f(unsigned long long val)
{
	return ff::FixedInt(val);
}

inline ff::FixedInt operator "" _f(long double val)
{
	return ff::FixedInt(val);
}

namespace std
{
	inline ff::FixedInt abs(ff::FixedInt val)
	{
		return val.abs();
	}

	inline ff::FixedInt ceil(ff::FixedInt val)
	{
		return val.ceil();
	}

	inline ff::FixedInt floor(ff::FixedInt val)
	{
		return val.floor();
	}

	inline ff::FixedInt trunc(ff::FixedInt val)
	{
		return val.trunc();
	}

	inline void swap(ff::FixedInt& lhs, ff::FixedInt& rhs)
	{
		lhs.swap(rhs);
	}

	inline ff::FixedInt copysign(ff::FixedInt val, ff::FixedInt sign)
	{
		return val.copysign(sign);
	}

	inline ff::FixedInt sqrt(ff::FixedInt val)
	{
		return std::sqrt((float)val);
	}
}
