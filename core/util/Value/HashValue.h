#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class HashValue : public ff::Value
	{
	public:
		UTIL_API HashValue(ff::hash_t value);

		UTIL_API ff::hash_t GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::hash_t value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		HashValue();

		ff::hash_t _value;
	};

	class HashValueType : public ff::ValueTypeBase<HashValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
