#pragma once

namespace ff
{
	class XamlGlobalState;

	class XamlProvider : public Noesis::XamlProvider
	{
	public:
		XamlProvider(XamlGlobalState* globals);
		virtual ~XamlProvider() override;

		virtual Noesis::Ptr<Noesis::Stream> LoadXaml(const char* uri) override;

	private:
		XamlGlobalState* _globals;
	};
}
