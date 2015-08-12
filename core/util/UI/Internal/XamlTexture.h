#pragma once

#include "Resource/ResourceValue.h"

namespace ff
{
	class ITexture;

	class XamlTexture : public Noesis::Texture
	{
	public:
		XamlTexture(ff::AutoResourceValue resource, ff::StringRef name);
		XamlTexture(ff::ITexture* texture, ff::StringRef name);
		virtual ~XamlTexture() override;

		static XamlTexture* Get(Noesis::Texture* texture);
		ff::ITexture* GetTexture() const;

		virtual uint32_t GetWidth() const override;
		virtual uint32_t GetHeight() const override;
		virtual bool HasMipMaps() const override;
		virtual bool IsInverted() const override;

	private:
		ff::AutoResourceValue _resource;
		ff::ComPtr<ff::ITexture> _texture;
		ff::String _name;
	};
}
