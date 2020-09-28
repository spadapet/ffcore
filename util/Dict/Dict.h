#pragma once

#include "Dict/SmallDict.h"
#include "Value/ValueType.h"

namespace ff
{
	class Dict
	{
	public:
		UTIL_API Dict();
		UTIL_API Dict(const Dict& rhs);
		UTIL_API Dict(Dict&& rhs);
		UTIL_API ~Dict();

		UTIL_API const Dict& operator=(const Dict& rhs);
		UTIL_API const Dict& operator=(Dict&& rhs);
		UTIL_API bool operator==(const Dict& rhs) const;

		UTIL_API void Add(const Dict& rhs);
		UTIL_API void Merge(const Dict& rhs);
		UTIL_API void ExpandSavedDictValues();

		UTIL_API size_t Size() const;
		UTIL_API bool IsEmpty() const;
		UTIL_API void Reserve(size_t count);
		UTIL_API void Clear();

		// Values
		UTIL_API void SetValue(ff::StringRef name, const ff::Value* value);
		UTIL_API ff::ValuePtr GetValue(ff::StringRef name) const;
		UTIL_API ff::Vector<ff::String> GetAllNames(bool sorted = false) const;

		template<typename T, typename... Args> void Set(ff::StringRef name, Args&&... args);
		template<typename T> auto Get(ff::StringRef name) const -> typename std::remove_reference<decltype(((T*)0)->GetValue())>::type;
		template<typename T, typename... Args> auto Get(ff::StringRef name, Args&&... defaultValue) const -> typename std::remove_reference<decltype(((T*)0)->GetValue())>::type;

		UTIL_API void DebugDump() const;

	private:
		ff::ValuePtr GetPathValue(ff::StringRef path) const;
		void InternalGetAllNames(ff::Set<ff::String>& names) const;
		void CheckSize();
		ff::StringCache& GetAtomizer() const;

		typedef ff::Map<ff::hash_t, ff::ValuePtr, ff::NonHasher<ff::hash_t>> PropsMap;

		std::unique_ptr<PropsMap> _propsLarge;
		ff::SmallDict _propsSmall;
	};
}

template<typename T, typename... Args>
void ff::Dict::Set(ff::StringRef name, Args&&... args)
{
	SetValue(name, ff::Value::New<T>(std::forward<Args>(args)...));
}

template<typename T>
auto ff::Dict::Get(ff::StringRef name) const -> typename std::remove_reference<decltype(((T*)0)->GetValue())>::type
{
	ff::ValuePtr value = GetValue(name)->Convert<T>();
	if (!value)
	{
		value = ff::Value::NewDefault<T>();
	}

	return value->GetValue<T>();
}

template<typename T, typename... Args>
auto ff::Dict::Get(ff::StringRef name, Args&&... defaultValue) const -> typename std::remove_reference<decltype(((T*)0)->GetValue())>::type
{
	ff::ValuePtr value = GetValue(name)->Convert<T>();
	if (!value)
	{
		value = ff::Value::New<T>(std::forward<Args>(defaultValue)...);
	}

	return value->GetValue<T>();
}
