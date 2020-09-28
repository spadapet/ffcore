#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::PointFloatValue::PointFloatValue(const ff::PointFloat& value)
	: _value(value)
{
}

const ff::PointFloat& ff::PointFloatValue::GetValue() const
{
	return _value;
}

ff::Value* ff::PointFloatValue::GetStaticValue(const ff::PointFloat& value)
{
	return value.IsNull() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::PointFloatValue::GetStaticDefaultValue()
{
	static ff::PointFloatValue s_zero(ff::PointFloat::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::PointFloatValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::PointFloat& src = value->GetValue<PointFloatValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %g, %g ]", src.x, src.y));
	}

	return nullptr;
}

ff::ValuePtr ff::PointFloatValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 2)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<FloatValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<FloatValue>();

		if (v0 && v1)
		{
			return ff::Value::New<PointFloatValue>(
				ff::PointFloat(v0->GetValue<FloatValue>(), v1->GetValue<FloatValue>()));
		}
	}

	return nullptr;
}

bool ff::PointFloatValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::PointFloatValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 2)
	{
		return ff::Value::New<FloatValue>(value->GetValue<PointFloatValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::PointFloatValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 2;
}

ff::ValuePtr ff::PointFloatValueType::Load(ff::IDataReader* stream) const
{
	ff::PointFloat data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<PointFloatValue>(data);
}

bool ff::PointFloatValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::PointFloat& data = value->GetValue<PointFloatValue>();
	return ff::SaveData(stream, data);
}
