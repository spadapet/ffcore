#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class FloatValue : public ff::Value
	{
	public:
		UTIL_API FloatValue(float value);

		UTIL_API float GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(float value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		float _value;
	};

	class FloatValueType : public ff::ValueTypeBase<FloatValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
