#pragma once

#include "Value/StringValue.h"

namespace ff
{
	class IValueType
	{
	public:
		UTIL_API virtual ~IValueType();

		// Types
		virtual bool Compare(const ff::Value* value1, const ff::Value* value2) const = 0;
		template<typename T> ff::ValuePtr ConvertTo(const ff::Value* value) const;
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const = 0;
		virtual ff::ValuePtr ConvertFrom(const ff::Value* otherValue) const = 0;

		// Maps
		virtual bool CanHaveNamedChildren() const = 0;
		virtual ff::ValuePtr GetNamedChild(const ff::Value* value, ff::StringRef name) const = 0;
		virtual ff::Vector<ff::String> GetChildNames(const ff::Value* value) const = 0;

		// Arrays
		virtual bool CanHaveIndexedChildren() const = 0;
		virtual ff::ValuePtr GetIndexChild(const ff::Value* value, size_t index) const = 0;
		virtual size_t GetIndexChildCount(const ff::Value* value) const = 0;

		// Objects
		virtual ff::ComPtr<IUnknown> GetComObject(const ff::Value* value) const = 0;

		// Persist
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const = 0;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const = 0;
		virtual ff::String Print(const ff::Value* value) = 0;
	};

	template<typename T>
	class ValueTypeBase : public IValueType
	{
	public:
		virtual bool Compare(const Value* value1, const ff::Value* value2) const override;
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr ConvertFrom(const ff::Value* otherValue) const override;

		virtual bool CanHaveNamedChildren() const override;
		virtual ff::ValuePtr GetNamedChild(const ff::Value* value, ff::StringRef name) const override;
		virtual ff::Vector<ff::String> GetChildNames(const ff::Value* value) const override;

		virtual bool CanHaveIndexedChildren() const override;
		virtual ff::ValuePtr GetIndexChild(const ff::Value* value, size_t index) const override;
		virtual size_t GetIndexChildCount(const ff::Value* value) const override;

		virtual ff::ComPtr<IUnknown> GetComObject(const ff::Value* value) const override;

		virtual ff::String Print(const ff::Value* value) override;
	};
}

template<typename T>
ff::ValuePtr ff::IValueType::ConvertTo(const ff::Value* value) const
{
	return ConvertTo(value, typeid(T));
}

template<typename T>
bool ff::ValueTypeBase<T>::Compare(const ff::Value* value1, const ff::Value* value2) const
{
	return value1->GetValue<T>() == value2->GetValue<T>();
}

template<typename T>
ff::ValuePtr ff::ValueTypeBase<T>::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

template<typename T>
ff::ValuePtr ff::ValueTypeBase<T>::ConvertFrom(const ff::Value* otherValue) const
{
	return nullptr;
}

template<typename T>
bool ff::ValueTypeBase<T>::CanHaveNamedChildren() const
{
	return false;
}

template<typename T>
ff::ValuePtr ff::ValueTypeBase<T>::GetNamedChild(const ff::Value* value, ff::StringRef name) const
{
	return nullptr;
}

template<typename T>
ff::Vector<ff::String> ff::ValueTypeBase<T>::GetChildNames(const ff::Value* value) const
{
	return ff::Vector<ff::String>();
}

template<typename T>
bool ff::ValueTypeBase<T>::CanHaveIndexedChildren() const
{
	return false;
}

template<typename T>
ff::ValuePtr ff::ValueTypeBase<T>::GetIndexChild(const ff::Value* value, size_t index) const
{
	return nullptr;
}

template<typename T>
size_t ff::ValueTypeBase<T>::GetIndexChildCount(const ff::Value* value) const
{
	return 0;
}

template<typename T>
ff::ComPtr<IUnknown> ff::ValueTypeBase<T>::GetComObject(const ff::Value* value) const
{
	return nullptr;
}

template<typename T>
ff::String ff::ValueTypeBase<T>::Print(const ff::Value* value)
{
	ff::ValuePtr stringValue = value->Convert<ff::StringValue>();
	if (stringValue)
	{
		return stringValue->GetValue<ff::StringValue>();
	}

	if (value->CanHaveIndexedChildren())
	{
		return ff::String::format_new(L"<%s[%lu]>", value->GetTypeName().c_str(), value->GetIndexChildCount());
	}

	if (value->CanHaveNamedChildren())
	{
		return ff::String::format_new(L"<%s[%lu]>", value->GetTypeName().c_str(), value->GetChildNames().Size());
	}

	return ff::String::format_new(L"<%s>", value->GetTypeName().c_str());
}
