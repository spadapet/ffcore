#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::RectFixedIntValue::RectFixedIntValue(const ff::RectFixedInt& value)
	: _value(value)
{
}

const ff::RectFixedInt& ff::RectFixedIntValue::GetValue() const
{
	return _value;
}

ff::Value* ff::RectFixedIntValue::GetStaticValue(const ff::RectFixedInt& value)
{
	return value.IsZeros() ? GetStaticDefaultValue() : nullptr;
}

ff::Value* ff::RectFixedIntValue::GetStaticDefaultValue()
{
	static ff::RectFixedIntValue s_zero(ff::RectFixedInt::Zeros());
	return &s_zero;
}

ff::ValuePtr ff::RectFixedIntValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	const ff::RectFixedInt& src = value->GetValue<RectFixedIntValue>();

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"[ %g, %g, %g, %g ]",
			(double)src.left, (double)src.top, (double)src.right, (double)src.bottom));
	}

	return nullptr;
}

ff::ValuePtr ff::RectFixedIntValueType::ConvertFrom(const ff::Value* otherValue) const
{
	if (otherValue->GetIndexChildCount() == 4)
	{
		ff::ValuePtr v0 = otherValue->GetIndexChild(0)->Convert<FixedIntValue>();
		ff::ValuePtr v1 = otherValue->GetIndexChild(1)->Convert<FixedIntValue>();
		ff::ValuePtr v2 = otherValue->GetIndexChild(2)->Convert<FixedIntValue>();
		ff::ValuePtr v3 = otherValue->GetIndexChild(3)->Convert<FixedIntValue>();

		if (v0 && v1 && v2 && v3)
		{
			return ff::Value::New<RectFixedIntValue>(ff::RectFixedInt(
				v0->GetValue<FixedIntValue>(),
				v1->GetValue<FixedIntValue>(),
				v2->GetValue<FixedIntValue>(),
				v3->GetValue<FixedIntValue>()));
		}
	}

	return nullptr;
}

bool ff::RectFixedIntValueType::CanHaveIndexedChildren() const
{
	return true;
}

ff::ValuePtr ff::RectFixedIntValueType::GetIndexChild(const ff::Value* value, size_t index) const
{
	if (index < 4)
	{
		return ff::Value::New<FixedIntValue>(value->GetValue<RectFixedIntValue>().arr[index]);
	}

	return nullptr;
}

size_t ff::RectFixedIntValueType::GetIndexChildCount(const ff::Value* value) const
{
	return 4;
}

ff::ValuePtr ff::RectFixedIntValueType::Load(ff::IDataReader* stream) const
{
	ff::RectFixedInt data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<RectFixedIntValue>(data);
}

bool ff::RectFixedIntValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	const ff::RectFixedInt& data = value->GetValue<RectFixedIntValue>();
	return ff::SaveData(stream, data);
}
