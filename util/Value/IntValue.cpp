#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::IntValue::IntValue()
{
}

ff::IntValue::IntValue(int value)
	: _value(value)
{
}

int ff::IntValue::GetValue() const
{
	return _value;
}

ff::Value* ff::IntValue::GetStaticValue(int value)
{
	noAssertRetVal(value >= -100 && value <= 200, nullptr);

	static bool s_init = false;
	static IntValue s_values[301];
	static ff::Mutex s_mutex;

	if (!s_init)
	{
		ff::LockMutex lock(s_mutex);
		if (!s_init)
		{
			for (int i = 0; i < 301; i++)
			{
				s_values[i]._value = i - 100;
			}

			s_init = true;
		}
	}

	return &s_values[value + 100];
}

ff::Value* ff::IntValue::GetStaticDefaultValue()
{
	return GetStaticValue(0);
}

ff::ValuePtr ff::IntValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	int src = value->GetValue<IntValue>();

	if (type == typeid(BoolValue))
	{
		return ff::Value::New<BoolValue>(src != 0);
	}

	if (type == typeid(DoubleValue))
	{
		return ff::Value::New<DoubleValue>((double)src);
	}

	if (type == typeid(FixedIntValue))
	{
		return ff::Value::New<FixedIntValue>(src);
	}

	if (type == typeid(FloatValue))
	{
		return ff::Value::New<FloatValue>((float)src);
	}

	if (type == typeid(SizeValue))
	{
		if (src >= 0)
		{
			return ff::Value::New<SizeValue>((size_t)src);
		}
		else if (src == -1)
		{
			return ff::Value::New<SizeValue>(ff::INVALID_SIZE);
		}
	}

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"%d", src));
	}

	return nullptr;
}

ff::ValuePtr ff::IntValueType::Load(ff::IDataReader* stream) const
{
	int data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<IntValue>(data);
}

bool ff::IntValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	int data = value->GetValue<IntValue>();
	return ff::SaveData(stream, data);
}
