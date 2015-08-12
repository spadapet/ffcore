#pragma once

#include "Dict/Dict.h"
#include "Value/ValueType.h"

namespace ff
{
	class DictValue : public ff::Value
	{
	public:
		UTIL_API DictValue(ff::Dict&& value);

		UTIL_API const ff::Dict& GetValue() const;
		UTIL_API static ff::Value* GetStaticValue(ff::Dict&& value);
		UTIL_API static ff::Value* GetStaticDefaultValue();

	private:
		ff::Dict _value;
	};

	class DictValueType : public ff::ValueTypeBase<DictValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr GetNamedChild(const ff::Value* value, ff::StringRef name) const override;
		virtual bool CanHaveNamedChildren() const override;
		virtual ff::Vector<ff::String> GetChildNames(const ff::Value* value) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
