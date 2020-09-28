#include "pch.h"
#include "Data/DataPersist.h"
#include "Value/Values.h"

ff::String HashToString(ff::hash_t hash);

ff::HashValue::HashValue()
{
}

ff::HashValue::HashValue(ff::hash_t value)
	: _value(value)
{
}

ff::hash_t ff::HashValue::GetValue() const
{
	return _value;
}

ff::Value* ff::HashValue::GetStaticValue(ff::hash_t value)
{
	noAssertRetVal(value < 256, nullptr);

	static bool s_init = false;
	static HashValue s_values[256];
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

ff::Value* ff::HashValue::GetStaticDefaultValue()
{
	return GetStaticValue(0);
}

ff::ValuePtr ff::HashValueType::ConvertTo(const ff::Value* value, std::type_index type) const
{
	ff::hash_t src = value->GetValue<HashValue>();

	if (type == typeid(BoolValue))
	{
		return ff::Value::New<BoolValue>(src != 0);
	}

	if (type == typeid(StringValue))
	{
		return ff::Value::New<ff::StringValue>(::HashToString(src));
	}

	return nullptr;
}

ff::ValuePtr ff::HashValueType::Load(ff::IDataReader* stream) const
{
	ff::hash_t data;
	assertRetVal(ff::LoadData(stream, data), false);
	return ff::Value::New<HashValue>(data);
}

bool ff::HashValueType::Save(const ff::Value* value, ff::IDataWriter* stream) const
{
	ff::hash_t data = value->GetValue<HashValue>();
	return ff::SaveData(stream, data);
}
