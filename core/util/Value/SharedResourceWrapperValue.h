#pragma once

#include "Resource/ResourceValue.h"
#include "Value/ValueType.h"

namespace ff
{
	class SharedResourceWrapperValue : public ff::Value
	{
	public:
		UTIL_API SharedResourceWrapperValue(const ff::SharedResourceValue& value);

		UTIL_API const ff::SharedResourceValue& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(const ff::SharedResourceValue& value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::SharedResourceValue _value;
	};

	class SharedResourceWrapperValueType : public ff::ValueTypeBase<SharedResourceWrapperValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
		virtual ff::ComPtr<IUnknown> GetComObject(const ff::Value* value) const override;
		virtual ff::String Print(const ff::Value* value) override;
	};
}
