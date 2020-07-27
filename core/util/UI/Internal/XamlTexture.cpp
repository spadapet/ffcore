#include "pch.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/Texture.h"
#include "UI/Internal/XamlTexture.h"
#include "Value/Values.h"

ff::XamlTexture::XamlTexture(ff::AutoResourceValue resource, ff::IPalette* palette, ff::StringRef name)
	: _resource(resource)
	, _palette(palette)
	, _name(name)
{
	assert(resource.DidInit());
}

ff::XamlTexture::XamlTexture(ff::ITexture* texture, ff::IPalette* palette, ff::StringRef name)
	: _texture(texture)
	, _palette(palette)
	, _name(name)
{
	assert(texture);
}

ff::XamlTexture::~XamlTexture()
{
}

ff::XamlTexture* ff::XamlTexture::Get(Noesis::Texture* texture)
{
	return static_cast<ff::XamlTexture*>(texture);
}

ff::ITexture* ff::XamlTexture::GetTexture() const
{
	if (!_texture && _resource.DidInit())
	{
		XamlTexture* self = const_cast<XamlTexture*>(this);
		ff::ValuePtr value = self->_resource.Flush();
		ff::ComPtr<IUnknown> comValue = value->GetValue<ff::ObjectValue>();

		self->_texture.QueryFrom(comValue);
		self->_resource = ff::AutoResourceValue();
	}

	assert(_texture);
	return _texture;
}

ff::IPalette* ff::XamlTexture::GetPalette() const
{
	return _palette;
}

uint32_t ff::XamlTexture::GetWidth() const
{
	ff::ITexture* texture = GetTexture();
	return static_cast<uint32_t>(texture ? texture->GetSize().x : 0);
}

uint32_t ff::XamlTexture::GetHeight() const
{
	ff::ITexture* texture = GetTexture();
	return static_cast<uint32_t>(texture ? texture->GetSize().y : 0);
}

bool ff::XamlTexture::HasMipMaps() const
{
	ff::ITexture* texture = GetTexture();
	return texture ? (texture->GetMipCount() > 1) : false;
}

bool ff::XamlTexture::IsInverted() const
{
	return false;
}
