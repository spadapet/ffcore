#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::PointFixedIntValue::PointFixedIntValue(const ff::PointFixedInt& value)
	: _value(value)
{
}

const ff::PointFixedInt& ff::PointFixedIntValue::GetValue() const
{
	return _value;
}

ff::Value* ff::PointFixedIntValue::GetStaticValue(const ff::PointFixedInt& value)
{
	return value.IsNull() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::PointFixedIntValue::GetStaticDefaultValue()
{
	static ff::PointFixedIntValue s_zero(ff::PointFixedInt::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::PointFixedIntValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::PointFixedInt& src = value->GetValue<PointFixedIntValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %g, %g ]", (double)src.x, (double)src.y));
	}

	return nullptr;
}

ff::ValuePtr ff::PointFixedIntValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 2)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<FixedIntValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<FixedIntValue>();

		if (v0 && v1)
		{
			return ff::Value::New<PointFixedIntValue>(
				ff::PointFixedInt(v0->GetValue<FixedIntValue>(), v1->GetValue<FixedIntValue>()));
		}
	}

	return nullptr;
}

bool ff::PointFixedIntValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::PointFixedIntValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 2)
	{
		return ff::Value::New<FixedIntValue>(value->GetValue<PointFixedIntValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::PointFixedIntValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 2;
}

ff::ValuePtr ff::PointFixedIntValueType::Load(ff::IDataReader* stream) const
{
	ff::PointFixedInt data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<PointFixedIntValue>(data);
}

bool ff::PointFixedIntValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::PointFixedInt& data = value->GetValue<PointFixedIntValue>();
	return ff::SaveData(stream, data);
}
