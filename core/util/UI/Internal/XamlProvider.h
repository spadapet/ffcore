#pragma once

#include "UI/Internal/XamlResourceCache.h"

namespace ff
{
	class XamlGlobalState;

	class XamlProvider : public Noesis::XamlProvider
	{
	public:
		XamlProvider(XamlGlobalState* globals);

		void Advance();
		virtual Noesis::Ptr<Noesis::Stream> LoadXaml(const char* uri) override;

	private:
		XamlResourceCache _cache;
	};
}
