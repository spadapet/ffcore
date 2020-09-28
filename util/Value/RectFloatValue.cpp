#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::RectFloatValue::RectFloatValue(const ff::RectFloat& value)
	: _value(value)
{
}

const ff::RectFloat& ff::RectFloatValue::GetValue() const
{
	return _value;
}

ff::Value* ff::RectFloatValue::GetStaticValue(const ff::RectFloat& value)
{
	return value.IsZeros() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::RectFloatValue::GetStaticDefaultValue()
{
	static ff::RectFloatValue s_zero(ff::RectFloat::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::RectFloatValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::RectFloat& src = value->GetValue<RectFloatValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %g, %g, %g, %g ]", src.left, src.top, src.right, src.bottom));
	}

	return nullptr;
}

ff::ValuePtr ff::RectFloatValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 4)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<FloatValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<FloatValue>();
		ff::ValuePtr v2 = otherValue->GetIndexChild(2)->Convert<FloatValue>();
		ff::ValuePtr v3 = otherValue->GetIndexChild(3)->Convert<FloatValue>();

		if (v0 && v1 && v2 && v3)
		{
			return ff::Value::New<RectFloatValue>(ff::RectFloat(
				v0->GetValue<FloatValue>(),
				v1->GetValue<FloatValue>(),
				v2->GetValue<FloatValue>(),
				v3->GetValue<FloatValue>()));
		}
	}

	return nullptr;
}

bool ff::RectFloatValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::RectFloatValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 4)
	{
		return ff::Value::New<FloatValue>(value->GetValue<RectFloatValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::RectFloatValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 4;
}

ff::ValuePtr ff::RectFloatValueType::Load(ff::IDataReader* stream) const
{
	ff::RectFloat data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<RectFloatValue>(data);
}

bool ff::RectFloatValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::RectFloat& data = value->GetValue<RectFloatValue>();
	return ff::SaveData(stream, data);
}
