#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class FixedIntValue : public ff::Value
	{
	public:
		UTIL_API FixedIntValue(ff::FixedInt value);

		UTIL_API ff::FixedInt GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::FixedInt value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::FixedInt _value;
	};

	class FixedIntValueType : public ff::ValueTypeBase<FixedIntValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
