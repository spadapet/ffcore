#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class DoubleValue : public ff::Value
	{
	public:
		UTIL_API DoubleValue(double value);

		UTIL_API double GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(double value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		double _value;
	};

	class DoubleValueType : public ff::ValueTypeBase<DoubleValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
