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
	: _cache(globals)
	, _palette(globals->GetPalette())
{
	ff::IGraphDevice* graph = globals->GetAppGlobals()->GetGraph();
	_placeholderTexture = graph->CreateTexture(ff::PointInt::Ones(), ff::TextureFormat::RGBA32);
	graph->CreateRenderTargetTexture(_placeholderTexture)->Clear(&ff::GetColorNone());
}

void ff::XamlTextureProvider::Advance()
{
	ff::Vector<ff::String, 32> changedNames;

	for (size_t i = _pendingTextures.Size(); i > 0; i--)
	{
		size_t index = i - 1;
		const Noesis::Ptr<XamlTexture>& texture = _pendingTextures[index];
		if (texture->GetTexture() != _placeholderTexture)
		{
			_pendingTextures.Delete(index);

			ff::StringRef uri = texture->GetName();
			_cache.GetResource(uri);

			ff::Vector<char> uri8 = ff::StringToUTF8(uri);
			RaiseTextureChanged(uri8.Data());
		}
	}

	_cache.Advance();
}

Noesis::TextureInfo ff::XamlTextureProvider::GetTextureInfo(const char* uri8)
{
	Noesis::TextureInfo info{};
	ff::String uri = ff::String::from_utf8(uri8) + ff::String::from_static(L".metadata");
	ff::AutoResourceValue res = _cache.GetResource(uri);
	ff::ValuePtr value = res.Flush();
	ff::ComPtr<IUnknown> comValue = value->GetValue<ff::ObjectValue>();

	ff::ComPtr<ff::ITextureMetadata> tm;
	if (tm.QueryFrom(comValue))
	{
		info.width = (unsigned int)tm->GetSize().x;
		info.height = (unsigned int)tm->GetSize().y;
	}

	assertSz(info.width && info.height, ff::String::format_new(L"XamlTextureProvider can't provide: %s", uri.c_str()).c_str());
	return info;
}

Noesis::Ptr<Noesis::Texture> ff::XamlTextureProvider::LoadTexture(const char* uri8, Noesis::RenderDevice* device)
{
	ff::String uri = ff::String::from_utf8(uri8);
	for (size_t i = 0; i < _pendingTextures.Size(); i++)
	{
		if (_pendingTextures[i]->GetName() == uri)
		{
			_pendingTextures.Delete(i);
			break;
		}
	}

	ff::SharedResourceValue res = _cache.GetResource(uri);
	Noesis::Ptr<XamlTexture> texture = *new XamlTexture(ff::AutoResourceValue(res), _placeholderTexture, _palette, uri);
	if (texture->GetTexture() == _placeholderTexture)
	{
		_pendingTextures.Push(texture);
	}

	return texture;
}
