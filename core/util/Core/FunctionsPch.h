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
}
