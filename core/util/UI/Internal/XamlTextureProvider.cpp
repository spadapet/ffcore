#include "pch.h"
#include "Graph/Texture/Texture.h"
#include "Resource/Resources.h"
#include "Resource/ResourceValue.h"
#include "UI/Internal/XamlTextureProvider.h"
#include "UI/Internal/XamlTexture.h"
#include "UI/XamlGlobalState.h"
#include "Value/Values.h"

ff::XamlTextureProvider::XamlTextureProvider(XamlGlobalState* globals)
	: _globals(globals)
{
}

ff::XamlTextureProvider::~XamlTextureProvider()
{
}

Noesis::TextureInfo ff::XamlTextureProvider::GetTextureInfo(const char* uri8)
{
	Noesis::TextureInfo info{};

	assertRetVal(_globals && uri8 && *uri8, info);
	ff::String uri = ff::String::from_utf8(uri8) + ff::String::from_static(L".metadata");

	ff::AutoResourceValue res = _globals->GetResourceAccess()->GetResource(uri);
	if (res.DidInit())
	{
		ff::ValuePtr value = res.Flush();
		ff::ComPtr<IUnknown> comValue = value->GetValue<ff::ObjectValue>();

		ff::ComPtr<ff::ITextureMetadata> tm;
		if (tm.QueryFrom(comValue))
		{
			info.width = (unsigned int)tm->GetSize().x;
			info.height = (unsigned int)tm->GetSize().y;
		}
	}

	assertSz(info.width && info.height, ff::String::format_new(L"XamlTextureProvider can't provide: %s", uri.c_str()).c_str());
	return info;
}

Noesis::Ptr<Noesis::Texture> ff::XamlTextureProvider::LoadTexture(const char* uri8, Noesis::RenderDevice* device)
{
	assertRetVal(_globals && uri8 && *uri8, nullptr);
	ff::String uri = ff::String::from_utf8(uri8);

	ff::AutoResourceValue res = _globals->GetResourceAccess()->GetResource(uri);
	if (res.DidInit())
	{
		return *new XamlTexture(res, _globals->GetPalette(), uri);
	}

	assertSz(false, ff::String::format_new(L"XamlTextureProvider can't provide: %s", uri.c_str()).c_str());
	return nullptr;
}
