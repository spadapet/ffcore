#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::RectIntValue::RectIntValue(const ff::RectInt& value)
	: _value(value)
{
}

const ff::RectInt& ff::RectIntValue::GetValue() const
{
	return _value;
}

ff::Value* ff::RectIntValue::GetStaticValue(const ff::RectInt& value)
{
	return value.IsZeros() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::RectIntValue::GetStaticDefaultValue()
{
	static ff::RectIntValue s_zero(ff::RectInt::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::RectIntValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::RectInt& src = value->GetValue<RectIntValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %d, %d, %d, %d ]", src.left, src.top, src.right, src.bottom));
	}

	return nullptr;
}

ff::ValuePtr ff::RectIntValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 4)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<IntValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<IntValue>();
		ff::ValuePtr v2 = otherValue->GetIndexChild(2)->Convert<IntValue>();
		ff::ValuePtr v3 = otherValue->GetIndexChild(3)->Convert<IntValue>();

		if (v0 && v1 && v2 && v3)
		{
			return ff::Value::New<RectIntValue>(ff::RectInt(
				v0->GetValue<IntValue>(),
				v1->GetValue<IntValue>(),
				v2->GetValue<IntValue>(),
				v3->GetValue<IntValue>()));
		}
	}

	return nullptr;
}

bool ff::RectIntValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::RectIntValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 4)
	{
		return ff::Value::New<IntValue>(value->GetValue<RectIntValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::RectIntValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 4;
}

ff::ValuePtr ff::RectIntValueType::Load(ff::IDataReader* stream) const
{
	ff::RectInt data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<RectIntValue>(data);
}

bool ff::RectIntValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::RectInt& data = value->GetValue<RectIntValue>();
	return ff::SaveData(stream, data);
}
