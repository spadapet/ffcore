#include "pch.h"
#include "Graph/Texture/Texture.h"
#include "UI/Internal/XamlTexture.h"
#include "UI/XamlGlobalState.h"
#include "Value/Values.h"

NS_IMPLEMENT_REFLECTION_(ff::XamlTexture);

ff::XamlTexture::XamlTexture(ff::AutoResourceValue&& resource, ff::XamlGlobalState* globals, ff::ITexture* placeholderTexture, ff::StringRef name)
	: _resource(std::move(resource))
	, _globals(globals)
	, _placeholderTexture(placeholderTexture)
	, _name(name)
{
	assert(resource.DidInit() && placeholderTexture);
}

ff::XamlTexture::XamlTexture(ff::ITexture* texture, ff::StringRef name)
	: _staticTexture(texture)
	, _globals(nullptr)
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

ff::StringRef ff::XamlTexture::GetName() const
{
	return _name;
}

ff::ITexture* ff::XamlTexture::GetTexture() const
{
	ff::ITexture* texture = _resource.DidInit() ? _resource.GetObject() : _staticTexture;
	return texture ? texture : _placeholderTexture;
}

ff::IPalette* ff::XamlTexture::GetPalette() const
{
	return _globals->GetPalette();
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
