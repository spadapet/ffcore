#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::PointIntValue::PointIntValue(const ff::PointInt& value)
	: _value(value)
{
}

const ff::PointInt& ff::PointIntValue::GetValue() const
{
	return _value;
}

ff::Value* ff::PointIntValue::GetStaticValue(const ff::PointInt& value)
{
	return value.IsNull() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::PointIntValue::GetStaticDefaultValue()
{
	static ff::PointIntValue s_zero(ff::PointInt::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::PointIntValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::PointInt& src = value->GetValue<PointIntValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %d, %d ]", src.x, src.y));
	}

	return nullptr;
}

ff::ValuePtr ff::PointIntValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 2)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<IntValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<IntValue>();

		if (v0 && v1)
		{
			return ff::Value::New<PointIntValue>(
				ff::PointInt(v0->GetValue<IntValue>(), v1->GetValue<IntValue>()));
		}
	}

	return nullptr;
}

bool ff::PointIntValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::PointIntValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 2)
	{
		return ff::Value::New<IntValue>(value->GetValue<PointIntValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::PointIntValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 2;
}

ff::ValuePtr ff::PointIntValueType::Load(ff::IDataReader* stream) const
{
	ff::PointInt data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<PointIntValue>(data);
}

bool ff::PointIntValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::PointInt& data = value->GetValue<PointIntValue>();
	return ff::SaveData(stream, data);
}
