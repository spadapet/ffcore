#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::IntStdVectorValue::IntStdVectorValue(std::vector<int>&& value)
	: _value(std::move(value))
{
}

const std::vector<int>& ff::IntStdVectorValue::GetValue() const
{
	return _value;
}

ff::Value* ff::IntStdVectorValue::GetStaticValue(std::vector<int>&& value)
{
	return value.empty() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::IntStdVectorValue::GetStaticDefaultValue()
{
	static IntStdVectorValue s_empty = std::vector<int>();
	return &s_empty;
}

ff::ValuePtr ff::IntStdVectorValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	return nullptr;
}

ff::ValuePtr ff::IntStdVectorValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->CanHaveIndexedChildren())
	{
		std::vector<int> vec;
		vec.reserve(otherValue->GetIndexChildCount());

		for (size_t i = 0; i < otherValue->GetIndexChildCount(); i++)
		{
			ff::ValuePtr val = otherValue->GetIndexChild(i)->Convert<IntValue>();
			noAssertRetVal(val, nullptr);

			vec.push_back(val->GetValue<IntValue>());
		}

		return ff::Value::New<IntStdVectorValue>(std::move(vec));
	}

	return nullptr;
}

bool ff::IntStdVectorValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::IntStdVectorValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	const std::vector<int>& values = value->GetValue<IntStdVectorValue>();
	return index < values.size() ? ff::Value::New<IntValue>(values[index]) : nullptr;
}

size_t ff::IntStdVectorValueType::GetIndexChildCount(const ff::Value* value) const
{
	const std::vector<int>& values = value->GetValue<IntStdVectorValue>();
	return values.size();
}

ff::ValuePtr ff::IntStdVectorValueType::Load(ff::IDataReader* stream) const
{
	DWORD size = 0;
	assertRetVal(ff::LoadData(stream, size), false);

	std::vector<int> vec;
	if (size)
	{
		vec.resize(size);
		assertRetVal(ff::LoadBytes(stream, vec.data(), size * sizeof(int)), false);
	}

	return ff::Value::New<IntStdVectorValue>(std::move(vec));
}

bool ff::IntStdVectorValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const std::vector<int>& src = value->GetValue<IntStdVectorValue>();
	assertRetVal(ff::SaveData(stream, (DWORD)src.size()), false);
	assertRetVal(src.empty() || ff::SaveBytes(stream, src.data(), src.size() * sizeof(int)), false);
	return true;
}
