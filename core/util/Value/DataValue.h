#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class IData;

	class DataValue : public ff::Value
	{
	public:
		UTIL_API DataValue(ff::IData* value);
		UTIL_API DataValue(const void* data, size_t size);

		UTIL_API const ff::ComPtr<ff::IData>& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::IData* value);
		UTIL_API static ff::Value* GetStaticValue(const void* data, size_t size);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::ComPtr<ff::IData> _value;
	};

	class DataValueType : public ff::ValueTypeBase<DataValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
		virtual ff::ComPtr<IUnknown> GetComObject(const ff::Value* value) const override;
		virtual ff::String Print(const ff::Value* value) override;
	};
}
