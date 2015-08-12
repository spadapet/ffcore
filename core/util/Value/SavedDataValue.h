#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class ISavedData;

	class SavedDataValue : public ff::Value
	{
	public:
		UTIL_API SavedDataValue(ff::ISavedData* value);

		UTIL_API const ff::ComPtr<ff::ISavedData>& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::ISavedData* value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::ComPtr<ff::ISavedData> _value;
	};

	class SavedDataValueType : public ff::ValueTypeBase<SavedDataValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
		virtual ff::ComPtr<IUnknown> GetComObject(const ff::Value* value) const override;
		virtual ff::String Print(const ff::Value* value) override;

		static ff::ValuePtr Load(ff::IDataReader* stream, bool savedDict);
		static ff::String Print(const ff::Value* value, ff::StringRef name);
	};
}
