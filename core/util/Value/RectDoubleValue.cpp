#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::RectDoubleValue::RectDoubleValue(const ff::RectDouble& value)
	: _value(value)
{
}

const ff::RectDouble& ff::RectDoubleValue::GetValue() const
{
	return _value;
}

ff::Value* ff::RectDoubleValue::GetStaticValue(const ff::RectDouble& value)
{
	return value.IsZeros() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::RectDoubleValue::GetStaticDefaultValue()
{
	static ff::RectDoubleValue s_zero(ff::RectDouble::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::RectDoubleValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::RectDouble& src = value->GetValue<RectDoubleValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %g, %g, %g, %g ]", src.left, src.top, src.right, src.bottom));
	}

	return nullptr;
}

ff::ValuePtr ff::RectDoubleValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 4)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<DoubleValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<DoubleValue>();
		ff::ValuePtr v2 = otherValue->GetIndexChild(2)->Convert<DoubleValue>();
		ff::ValuePtr v3 = otherValue->GetIndexChild(3)->Convert<DoubleValue>();

		if (v0 && v1 && v2 && v3)
		{
			return ff::Value::New<RectDoubleValue>(ff::RectDouble(
				v0->GetValue<DoubleValue>(),
				v1->GetValue<DoubleValue>(),
				v2->GetValue<DoubleValue>(),
				v3->GetValue<DoubleValue>()));
		}
	}

	return nullptr;
}

bool ff::RectDoubleValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::RectDoubleValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 4)
	{
		return ff::Value::New<DoubleValue>(value->GetValue<RectDoubleValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::RectDoubleValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 4;
}

ff::ValuePtr ff::RectDoubleValueType::Load(ff::IDataReader* stream) const
{
	ff::RectDouble data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<RectDoubleValue>(data);
}

bool ff::RectDoubleValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::RectDouble& data = value->GetValue<RectDoubleValue>();
	return ff::SaveData(stream, data);
}
