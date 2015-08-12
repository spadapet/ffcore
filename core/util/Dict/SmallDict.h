#pragma once

namespace ff
{
	class StringCache;
	class Value;

	// Implements a key/value dictionary using a simple single array. It's faster
	// than a hash table for "small" dictionaries.
	class SmallDict
	{
	public:
		UTIL_API SmallDict();
		UTIL_API SmallDict(const SmallDict& rhs);
		UTIL_API SmallDict(SmallDict&& rhs);
		UTIL_API ~SmallDict();

		UTIL_API SmallDict& operator=(const SmallDict& rhs);
		UTIL_API SmallDict& operator=(SmallDict&& rhs);

		UTIL_API size_t Size() const;
		UTIL_API size_t Allocated() const;
		UTIL_API ff::String KeyAt(size_t index) const;
		UTIL_API ff::hash_t KeyHashAt(size_t index) const;
		UTIL_API const ff::Value* ValueAt(size_t index) const;
		UTIL_API const ff::Value* GetValue(ff::StringRef key) const;
		UTIL_API size_t IndexOf(ff::StringRef key) const;

		UTIL_API void Add(ff::StringRef key, const ff::Value* value); // super fast, no dupe check
		UTIL_API void Set(ff::StringRef key, const ff::Value* value);
		UTIL_API void SetAt(size_t index, const ff::Value* value);
		UTIL_API void Remove(ff::StringRef key);
		UTIL_API void RemoveAt(size_t index);
		UTIL_API void Reserve(size_t newAllocated, bool allowEmptySpace = true);
		UTIL_API void Clear();

	private:
		ff::StringCache& GetAtomizer() const;

		struct Entry
		{
			ff::hash_t hash;
			const ff::Value* value;
		};

		struct Data
		{
			size_t allocated;
			size_t size;
			Entry entries[1];
		};

		Data* _data;
	};
}
