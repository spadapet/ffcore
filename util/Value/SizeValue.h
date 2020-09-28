#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class SizeValue : public ff::Value
	{
	public:
		UTIL_API SizeValue(size_t value);

		UTIL_API size_t GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(size_t value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		SizeValue();

		size_t _value;
	};

	class SizeValueType : public ff::ValueTypeBase<SizeValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
