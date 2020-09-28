#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::PointDoubleValue::PointDoubleValue(const ff::PointDouble& value)
	: _value(value)
{
}

const ff::PointDouble& ff::PointDoubleValue::GetValue() const
{
	return _value;
}

ff::Value* ff::PointDoubleValue::GetStaticValue(const ff::PointDouble& value)
{
	return value.IsNull() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::PointDoubleValue::GetStaticDefaultValue()
{
	static ff::PointDoubleValue s_zero(ff::PointDouble::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::PointDoubleValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::PointDouble& src = value->GetValue<PointDoubleValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %g, %g ]", src.x, src.y));
	}

	return nullptr;
}

ff::ValuePtr ff::PointDoubleValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 2)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<DoubleValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<DoubleValue>();

		if (v0 && v1)
		{
			return ff::Value::New<PointDoubleValue>(
				ff::PointDouble(v0->GetValue<DoubleValue>(), v1->GetValue<DoubleValue>()));
		}
	}

	return nullptr;
}

bool ff::PointDoubleValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::PointDoubleValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 2)
	{
		return ff::Value::New<DoubleValue>(value->GetValue<PointDoubleValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::PointDoubleValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 2;
}

ff::ValuePtr ff::PointDoubleValueType::Load(ff::IDataReader* stream) const
{
	ff::PointDouble data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<PointDoubleValue>(data);
}

bool ff::PointDoubleValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::PointDouble& data = value->GetValue<PointDoubleValue>();
	return ff::SaveData(stream, data);
}
