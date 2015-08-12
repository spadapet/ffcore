#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::SizeValue::SizeValue()
{
}

ff::SizeValue::SizeValue(size_t value)
	: _value(value)
{
}

size_t ff::SizeValue::GetValue() const
{
	return _value;
}

ff::Value* ff::SizeValue::GetStaticValue(size_t value)
{
	noAssertRetVal(value < 256, nullptr);

	static bool s_init = false;
	static SizeValue s_values[256];
	static ff::Mutex s_mutex;

	if (!s_init)
	{
		ff::LockMutex lock(s_mutex);
		if (!s_init)
		{
			for (size_t i = 0; i < 256; i++)
			{
				s_values[i]._value = i;
			}

			s_init = true;
		}
	}

	return &s_values[value];
}

ff::Value* ff::SizeValue::GetStaticDefaultValue()
{
	return GetStaticValue(0);
}

ff::ValuePtr ff::SizeValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	size_t src = value->GetValue<SizeValue>();

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
		return ff::Value::New<FixedIntValue>((unsigned long long)src);
	}

	if (type == typeid(FloatValue))
	{
		return ff::Value::New<FloatValue>((float)src);
	}

	if (type == typeid(IntValue))
	{
		return ff::Value::New<IntValue>((int)src);
	}

	if (type == typeid(StringValue))
	{
		return ff::Value::New<StringValue>(ff::String::format_new(L"%d", src));
	}

	return nullptr;
}

ff::ValuePtr ff::SizeValueType::Load(ff::IDataReader* stream) const
{
	uint64_t data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<SizeValue>((size_t)data);
}

bool ff::SizeValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	uint64_t data = value->GetValue<SizeValue>();
	return ff::SaveData(stream, data);
}
