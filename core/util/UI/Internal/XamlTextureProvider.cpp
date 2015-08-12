#include "pch.h"
#include "UI/Internal/XamlTextureProvider.h"
#include "UI/Internal/XamlTexture.h"
#include "Resource/Resources.h"
#include "Resource/ResourceValue.h"
#include "UI/XamlGlobalState.h"

ff::XamlTextureProvider::XamlTextureProvider(XamlGlobalState* globals)
	: _globals(globals)
{
}

ff::XamlTextureProvider::~XamlTextureProvider()
{
}

Noesis::TextureInfo ff::XamlTextureProvider::GetTextureInfo(const char* uri)
{
	return Noesis::TextureInfo{};
}

Noesis::Ptr<Noesis::Texture> ff::XamlTextureProvider::LoadTexture(const char* uri8, Noesis::RenderDevice* device)
{
	assertRetVal(_globals && uri8 && *uri8, nullptr);
	ff::String uri = ff::String::from_utf8(uri8);

	ff::AutoResourceValue res = _globals->GetResourceAccess()->GetResource(uri);
	if (res.DidInit())
	{
		return *new XamlTexture(res, uri);
	}

	assertSz(false, ff::String::format_new(L"XamlTextureProvider can't provide: %s", uri.c_str()).c_str());
	return nullptr;
}
