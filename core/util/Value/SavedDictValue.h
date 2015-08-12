#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class Dict;
	class ISavedData;

	class SavedDictValue : public ff::Value
	{
	public:
		UTIL_API SavedDictValue(ff::ISavedData* value);
		UTIL_API SavedDictValue(const ff::Dict& value, bool compress);

		UTIL_API const ff::ComPtr<ff::ISavedData>& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::ISavedData* value);
		UTIL_API static ff::Value* GetStaticValue(const ff::Dict& value, bool compress);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::ComPtr<ff::ISavedData> _value;
	};

	class SavedDictValueType : public ff::ValueTypeBase<SavedDictValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
		virtual ff::ComPtr<IUnknown> GetComObject(const ff::Value* value) const override;
		virtual ff::String Print(const ff::Value* value) override;
	};
}
