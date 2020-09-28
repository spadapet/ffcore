#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class IntValue : public ff::Value
	{
	public:
		UTIL_API IntValue(int value);

		UTIL_API int GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(int value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		IntValue();

		int _value;
	};

	class IntValueType : public ff::ValueTypeBase<IntValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
