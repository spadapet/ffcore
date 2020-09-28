#include "pch.h"
#include "UI/Utility/Converters.h"

NS_IMPLEMENT_REFLECTION(ff::BoolToVisibleConverter, "ff.BoolToVisibleConverter")
{
}

bool ff::BoolToVisibleConverter::TryConvert(Noesis::BaseComponent* value, const Noesis::Type* targetType, Noesis::BaseComponent* parameter, Noesis::Ptr<Noesis::BaseComponent>& result)
{
	if (Noesis::Boxing::CanUnbox<bool>(value))
	{
		bool visible = Noesis::Boxing::Unbox<bool>(value);
		result = Noesis::Boxing::Box<Noesis::Visibility>(visible ? Noesis::Visibility_Visible : Noesis::Visibility_Collapsed);
		return true;
	}

	return false;
}

NS_IMPLEMENT_REFLECTION(ff::BoolToCollapsedConverter, "ff.BoolToCollapsedConverter")
{
}

bool ff::BoolToCollapsedConverter::TryConvert(Noesis::BaseComponent* value, const Noesis::Type* targetType, Noesis::BaseComponent* parameter, Noesis::Ptr<Noesis::BaseComponent>& result)
{
	if (Noesis::Boxing::CanUnbox<bool>(value))
	{
		bool visible = !Noesis::Boxing::Unbox<bool>(value);
		result = Noesis::Boxing::Box<Noesis::Visibility>(visible ? Noesis::Visibility_Visible : Noesis::Visibility_Collapsed);
		return true;
	}

	return false;
}

NS_IMPLEMENT_REFLECTION(ff::BoolToObjectConverter, "ff.BoolToObjectConverter")
{
	NsProp("TrueValue", &BoolToObjectConverter::GetTrueValue, &BoolToObjectConverter::SetTrueValue);
	NsProp("FalseValue", &BoolToObjectConverter::GetFalseValue, &BoolToObjectConverter::SetFalseValue);
}

bool ff::BoolToObjectConverter::TryConvert(Noesis::BaseComponent* value, const Noesis::Type* targetType, Noesis::BaseComponent* parameter, Noesis::Ptr<Noesis::BaseComponent>& result)
{
	if (Noesis::Boxing::CanUnbox<bool>(value))
	{
		bool value2 = Noesis::Boxing::Unbox<bool>(value);
		result = value2 ? _trueValue : _falseValue;
		return true;
	}

	return false;
}

Noesis::BaseComponent* ff::BoolToObjectConverter::GetTrueValue() const
{
	return _trueValue;
}

void ff::BoolToObjectConverter::SetTrueValue(Noesis::BaseComponent* value)
{
	_trueValue.Reset(value);
}

Noesis::BaseComponent* ff::BoolToObjectConverter::GetFalseValue() const
{
	return _falseValue;
}

void ff::BoolToObjectConverter::SetFalseValue(Noesis::BaseComponent* value)
{
	_falseValue.Reset(value);
}
