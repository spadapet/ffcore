#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class RectIntValue : public ff::Value
	{
	public:
		UTIL_API RectIntValue(const ff::RectInt& value);

		UTIL_API const ff::RectInt& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(const ff::RectInt& value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::RectInt _value;
	};

	class RectIntValueType : public ff::ValueTypeBase<RectIntValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr ConvertFrom(const ff::Value* otherValue) const override;

		virtual bool CanHaveIndexedChildren() const override;
		virtual ff::ValuePtr GetIndexChild(const ff::Value* value, size_t index) const override;
		virtual size_t GetIndexChildCount(const ff::Value* value) const override;

		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
