#pragma once

namespace ff
{
	class UTIL_API BoolToVisibleConverter : public Noesis::BaseValueConverter
	{
	public:
		virtual bool TryConvert(Noesis::BaseComponent* value, const Noesis::Type* targetType, Noesis::BaseComponent* parameter, Noesis::Ptr<Noesis::BaseComponent>& result) override;

	private:
		NS_DECLARE_REFLECTION(BoolToVisibleConverter, Noesis::BaseValueConverter);
	};

	class UTIL_API BoolToCollapsedConverter : public Noesis::BaseValueConverter
	{
	public:
		virtual bool TryConvert(Noesis::BaseComponent* value, const Noesis::Type* targetType, Noesis::BaseComponent* parameter, Noesis::Ptr<Noesis::BaseComponent>& result) override;

	private:
		NS_DECLARE_REFLECTION(BoolToCollapsedConverter, Noesis::BaseValueConverter);
	};

	class UTIL_API BoolToObjectConverter : public Noesis::BaseValueConverter
	{
	public:
		virtual bool TryConvert(Noesis::BaseComponent* value, const Noesis::Type* targetType, Noesis::BaseComponent* parameter, Noesis::Ptr<Noesis::BaseComponent>& result) override;

		Noesis::BaseComponent* GetTrueValue() const;
		void SetTrueValue(Noesis::BaseComponent* value);
		Noesis::BaseComponent* GetFalseValue() const;
		void SetFalseValue(Noesis::BaseComponent* value);

	private:
		Noesis::Ptr<Noesis::BaseComponent> _trueValue;
		Noesis::Ptr<Noesis::BaseComponent> _falseValue;

		NS_DECLARE_REFLECTION(BoolToObjectConverter, Noesis::BaseValueConverter);
	};
}
