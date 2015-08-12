#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class ValueVectorValue : public ff::Value
	{
	public:
		UTIL_API ValueVectorValue(ff::Vector<ff::ValuePtr>&& value);

		UTIL_API const ff::Vector<ff::ValuePtr>& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::Vector<ff::ValuePtr>&& value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::Vector<ff::ValuePtr> _value;
	};

	class ValueVectorValueType : public ff::ValueTypeBase<ValueVectorValue>
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
