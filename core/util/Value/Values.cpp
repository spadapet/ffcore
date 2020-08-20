#include "pch.h"
#include "Data/Data.h"
#include "Data/SavedData.h"
#include "Globals/ProcessStartup.h"
#include "Value/Values.h"

static ff::ProcessStartup RegisterDefaultTypes([](ff::ProcessGlobals& globals)
	{
		ff::Value::RegisterType<ff::BoolValue, ff::BoolValueType>();
		ff::Value::RegisterType<ff::DataValue, ff::DataValueType>();
		ff::Value::RegisterType<ff::DictValue, ff::DictValueType>();
		ff::Value::RegisterType<ff::DoubleValue, ff::DoubleValueType>();
		ff::Value::RegisterType<ff::DoubleVectorValue, ff::DoubleVectorValueType>();
		ff::Value::RegisterType<ff::FixedIntValue, ff::FixedIntValueType>();
		ff::Value::RegisterType<ff::FixedIntVectorValue, ff::FixedIntVectorValueType>();
		ff::Value::RegisterType<ff::FloatValue, ff::FloatValueType>();
		ff::Value::RegisterType<ff::FloatVectorValue, ff::FloatVectorValueType>();
		ff::Value::RegisterType<ff::GuidValue, ff::GuidValueType>();
		ff::Value::RegisterType<ff::HashValue, ff::HashValueType>();
		ff::Value::RegisterType<ff::IntValue, ff::IntValueType>();
		ff::Value::RegisterType<ff::IntVectorValue, ff::IntVectorValueType>();
		ff::Value::RegisterType<ff::NullValue, ff::NullValueType>();
		ff::Value::RegisterType<ff::ObjectValue, ff::ObjectValueType>();
		ff::Value::RegisterType<ff::PointDoubleValue, ff::PointDoubleValueType>();
		ff::Value::RegisterType<ff::PointFixedIntValue, ff::PointFixedIntValueType>();
		ff::Value::RegisterType<ff::PointFloatValue, ff::PointFloatValueType>();
		ff::Value::RegisterType<ff::PointIntValue, ff::PointIntValueType>();
		ff::Value::RegisterType<ff::RectDoubleValue, ff::RectDoubleValueType>();
		ff::Value::RegisterType<ff::RectFixedIntValue, ff::RectFixedIntValueType>();
		ff::Value::RegisterType<ff::RectFloatValue, ff::RectFloatValueType>();
		ff::Value::RegisterType<ff::RectIntValue, ff::RectIntValueType>();
		ff::Value::RegisterType<ff::SavedDataValue, ff::SavedDataValueType>();
		ff::Value::RegisterType<ff::SavedDictValue, ff::SavedDictValueType>();
		ff::Value::RegisterType<ff::SharedResourceWrapperValue, ff::SharedResourceWrapperValueType>();
		ff::Value::RegisterType<ff::SizeValue, ff::SizeValueType>();
		ff::Value::RegisterType<ff::StringStdVectorValue, ff::StringStdVectorValueType>();
		ff::Value::RegisterType<ff::StringValue, ff::StringValueType>();
		ff::Value::RegisterType<ff::StringVectorValue, ff::StringVectorValueType>();
		ff::Value::RegisterType<ff::ValueVectorValue, ff::ValueVectorValueType>();
	});

static ff::ProcessShutdown UnregisterValueTypes([](ff::ProcessGlobals& globals)
	{
		ff::Value::UnregisterAllTypes();
	});
