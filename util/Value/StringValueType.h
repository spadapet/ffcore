#pragma once

#include "Value/ValueType.h"

namespace ff
{
	class StringValueType : public ff::ValueTypeBase<StringValue>
	{
	public:
		virtual ff::ValuePtr ConvertTo(const ff::Value* value, std::type_index type) const override;
		virtual ff::ValuePtr Load(ff::IDataReader* stream) const override;
		virtual bool Save(const ff::Value* value, ff::IDataWriter* stream) const override;
	};
}
