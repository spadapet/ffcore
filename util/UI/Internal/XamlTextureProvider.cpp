#include "pch.h"
#include "Globals/AppGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Texture/Palette.h"
#include "Graph/Texture/Texture.h"
#include "Resource/Resources.h"
#include "String/StringUtil.h"
#include "UI/Internal/XamlTextureProvider.h"
#include "UI/Internal/XamlTexture.h"
#include "UI/XamlGlobalState.h"
#include "Value/Values.h"

ff::XamlTextureProvider::XamlTextureProvider(XamlGlobalState* globals)
	: _globals(globals)
	, _cache(globals)
	, _palette(globals->GetPalette())
{
	ff::IGraphDevice* graph = globals->GetAppGlobals()->GetGraph();
	_placeholderTexture = graph->CreateTexture(ff::PointInt::Ones(), ff::TextureFormat::RGBA32);
	graph->CreateRenderTargetTexture(_placeholderTexture)->Clear(&ff::GetColorNone());
}

void ff::XamlTextureProvider::Advance()
{
	_cache.Advance();
}

Noesis::TextureInfo ff::XamlTextureProvider::GetTextureInfo(const char* uri8)
{
	ff::String uri = ff::String::from_utf8(uri8) + ff::String::from_static(L".metadata");
	ff::TypedResource<ff::ITextureMetadata> res(&_cache, uri);
	if (res.Flush())
	{
		ff::PointType<unsigned int> size = res->GetSize().ToType<unsigned int>();
		return Noesis::TextureInfo{ size.x, size.y };
	}

	assertSz(false, ff::String::format_new(L"Missing texture resource: %s", uri.c_str()).c_str());
	return Noesis::TextureInfo{};
}

Noesis::Ptr<Noesis::Texture> ff::XamlTextureProvider::LoadTexture(const char* uri8, Noesis::RenderDevice* device)
{
	ff::String uri = ff::String::from_utf8(uri8);
	ff::SharedResourceValue res = _cache.GetResource(uri);
	Noesis::Ptr<XamlTexture> texture = *new XamlTexture(ff::AutoResourceValue(res), _globals, _placeholderTexture, uri);
	return texture;
}
