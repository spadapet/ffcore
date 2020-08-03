#pragma once

#include "UI/Internal/XamlResourceCache.h"

namespace ff
{
	class IPalette;
	class ITexture;
	class XamlGlobalState;
	class XamlTexture;

	class XamlTextureProvider : public Noesis::TextureProvider
	{
	public:
		XamlTextureProvider(XamlGlobalState* globals);

		void Advance();
		virtual Noesis::TextureInfo GetTextureInfo(const char* uri) override;
		virtual Noesis::Ptr<Noesis::Texture> LoadTexture(const char* uri, Noesis::RenderDevice* device) override;

	private:
		XamlGlobalState* _globals;
		XamlResourceCache _cache;
		ff::ComPtr<ff::IPalette> _palette;
		ff::ComPtr<ff::ITexture> _placeholderTexture;
	};
}
