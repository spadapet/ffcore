#include "pch.h"
#include "Value/Values.h"

bool ValueTest()
{
	ff::ValuePtr nullValue1 = ff::Value::New<ff::NullValue>();
	ff::ValuePtr nullValue2 = ff::Value::New<ff::NullValue>();
	assertRetVal(nullValue1->Compare(nullValue2), false);
	assertRetVal(nullValue1 == nullValue2, false)

	ff::ValuePtr value = ff::Value::New<ff::BoolValue>(true);
	assertRetVal(value->IsType<ff::BoolValue>(), false);
	assertRetVal(value->GetValue<ff::BoolValue>(), false);

	ff::ValuePtr value2 = value->Convert<ff::DoubleValue>();
	assertRetVal(value2->IsType<ff::DoubleValue>(), false);
	assertRetVal(value2->GetValue<ff::DoubleValue>() == 1.0, false);

	return true;
}
