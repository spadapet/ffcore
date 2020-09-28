#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class NullValue : public ff::Value
	{
	public:
		UTIL_API NullValue();

		UTIL_API std::nullptr_t GetValue() const;
		UTIL_API static ff::Value* GetStaticValue();
		UTIL_API static ff::Value* GetStaticDefaultValue();
	};

	class NullValueType : public ff::ValueTypeBase<NullValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
