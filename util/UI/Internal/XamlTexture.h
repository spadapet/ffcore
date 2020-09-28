#pragma once

#include "Resource/ResourceValue.h"

namespace ff
{
	class IPalette;
	class ITexture;
	class XamlGlobalState;

	class XamlTexture : public Noesis::Texture
	{
	public:
		XamlTexture(ff::AutoResourceValue&& resource, ff::XamlGlobalState* globals, ff::ITexture* placeholderTexture, ff::StringRef name);
		XamlTexture(ff::ITexture* texture, ff::StringRef name);
		virtual ~XamlTexture() override;

		static XamlTexture* Get(Noesis::Texture* texture);
		ff::StringRef GetName() const;
		ff::ITexture* GetTexture() const;
		ff::IPalette* GetPalette() const;

		virtual uint32_t GetWidth() const override;
		virtual uint32_t GetHeight() const override;
		virtual bool HasMipMaps() const override;
		virtual bool IsInverted() const override;

	private:
		ff::XamlGlobalState* _globals;
		mutable ff::TypedResource<ff::ITexture> _resource;
		ff::ComPtr<ff::ITexture> _staticTexture;
		ff::ComPtr<ff::ITexture> _placeholderTexture;
		ff::String _name;

		NS_DECLARE_REFLECTION(ff::XamlTexture, Noesis::Texture);
	};
}
