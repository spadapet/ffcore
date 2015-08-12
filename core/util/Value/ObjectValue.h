#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class ObjectValue : public ff::Value
	{
	public:
		UTIL_API ObjectValue(IUnknown* value);

		UTIL_API const ff::ComPtr<IUnknown>& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(IUnknown* value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::ComPtr<IUnknown> _value;
	};

	class ObjectValueType : public ff::ValueTypeBase<ObjectValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
		virtual ff::ComPtr<IUnknown> GetComObject(const ff::Value* value) const override;
		virtual ff::String Print(const ff::Value* value) override;
	};
}
