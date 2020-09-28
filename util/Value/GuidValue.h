#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class GuidValue : public ff::Value
	{
	public:
		UTIL_API GuidValue(REFGUID value);

		UTIL_API REFGUID GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(REFGUID value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		GUID _value;
	};

	class GuidValueType : public ff::ValueTypeBase<GuidValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
