#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class BoolValue : public ff::Value
	{
	public:
		UTIL_API BoolValue(bool value);

		UTIL_API bool GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(bool value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		bool _value;
	};

	class BoolValueType : public ff::ValueTypeBase<BoolValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr ConvertFrom(const ff::Value* otherValue) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
