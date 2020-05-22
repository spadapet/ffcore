#pragma once

namespace ff
{
	class XamlGlobalState;

	class XamlFontProvider : public Noesis::CachedFontProvider
	{
	public:
		XamlFontProvider(XamlGlobalState* globals);
		virtual ~XamlFontProvider() override;

		virtual void ScanFolder(const char* folder) override;
		virtual Noesis::Ptr<Noesis::Stream> OpenFont(const char* folder, const char* filename) const override;
		virtual Noesis::FontSource MatchFont(const char* baseUri, const char* familyName, Noesis::FontWeight& weight, Noesis::FontStretch& stretch, Noesis::FontStyle& style) override;
		virtual bool FamilyExists(const char* baseUri, const char* familyName) override;

	private:
		XamlGlobalState* _globals;
	};
}
