#pragma once

#include "UI/Internal/XamlResourceCache.h"

namespace ff
{
	class XamlGlobalState;

	class XamlFontProvider : public Noesis::CachedFontProvider
	{
	public:
		XamlFontProvider(XamlGlobalState* globals);

		void Advance();
		virtual void ScanFolder(const char* folder) override;
		virtual Noesis::Ptr<Noesis::Stream> OpenFont(const char* folder, const char* filename) const override;

	private:
		XamlResourceCache _cache;
	};
}
