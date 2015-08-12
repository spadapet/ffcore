#pragma once

namespace ff
{
	struct MemoryStats
	{
		size_t total;
		size_t current;
		size_t maximum;
		size_t count;
	};

	MemoryStats GetMemoryAllocationStats();

	class AtScope
	{
	public:
		UTIL_API AtScope(AtScope&& rhs);
		UTIL_API AtScope(std::function<void()>&& closeFunc);
		UTIL_API AtScope(std::function<void()>&& openFunc, std::function<void()>&& closeFunc);
		UTIL_API ~AtScope();

		UTIL_API void Close();

	private:
		std::function<void()> _closeFunc;
	};

	// An object for using the global C++ allocator
	template<typename T, size_t TAlignment = 0>
	struct MemAllocator
	{
		static T* Malloc(size_t count)
		{
			return (T*)::_aligned_malloc(count * sizeof(T), TAlignment ? TAlignment : alignof(T));
		}

		static void Free(T* data)
		{
			::_aligned_free(data);
		}
	};
}
