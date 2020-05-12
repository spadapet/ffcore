#pragma once

namespace ff
{
	template<typename T>
	void ZeroObject(T& t)
	{
		std::memset(&t, 0, sizeof(T));
	}

	inline static size_t NearestPowerOfTwo(size_t num)
	{
		num = num ? num - 1 : 0;

		num |= num >> 1;
		num |= num >> 2;
		num |= num >> 4;
		num |= num >> 8;
		num |= num >> 16;
#ifdef _WIN64
		num |= num >> 32;
#endif

		return num + 1;
	}

	inline static size_t RoundUp(size_t num, size_t multiple)
	{
		return (num + multiple - 1) / multiple * multiple;
	}

	template<typename T>
	bool HasAllFlags(T state, T check)
	{
		typedef std::underlying_type_t<T> TI;
		return ((TI)state & (TI)check) == (TI)check;
	}

	template<typename T>
	bool HasAnyFlags(T state, T check)
	{
		typedef std::underlying_type_t<T> TI;
		return ((TI)state & (TI)check) != (TI)0;
	}

	template<typename T>
	T SetFlags(T state, T set, bool value)
	{
		typedef std::underlying_type_t<T> TI;
		const TI istate = (TI)state;
		const TI iset = (TI)set;
		const TI ival = (TI)value;

		return (T)((istate & ~iset) | (iset * ival));
	}

	template<typename T>
	T SetFlags(T state, T set)
	{
		typedef std::underlying_type_t<T> TI;
		return (T)((TI)state | (TI)set);
	}

	template<typename T>
	T ClearFlags(T state, T clear)
	{
		typedef std::underlying_type_t<T> TI;
		return (T)((TI)state & ~(TI)clear);
	}

	template<typename T>
	T CombineFlags(T f1, T f2)
	{
		typedef std::underlying_type_t<T> TI;
		return (T)((TI)f1 | (TI)f2);
	}

	template<typename T>
	T CombineFlags(T f1, T f2, T f3)
	{
		typedef std::underlying_type_t<T> TI;
		return (T)((TI)f1 | (TI)f2 | (TI)f3);
	}

	template<typename T>
	T CombineFlags(T f1, T f2, T f3, T f4)
	{
		typedef std::underlying_type_t<T> TI;
		return (T)((TI)f1 | (TI)f2 | (TI)f3 | (TI)f4);
	}
}
