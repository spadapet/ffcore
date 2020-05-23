#pragma once

#include "Entity/EntityDomain.h"
#include "Graph/RenderTarget/Viewport.h"
#include "Resource/ResourceValue.h"
#include "State/State.h"

namespace ff
{
	class IPalette;
	class IPaletteData;
	class IRenderDepth;
	class IRenderer;
	class ISprite;
	class ISpriteFont;
	class ISpriteList;
	class ITexture;
}

namespace TestPalette
{
	struct UpdateSystemEntry;
	struct RenderSystemEntry;
}

class TestPaletteState : public ff::State
{
public:
	TestPaletteState(ff::AppGlobals* globals);

	virtual std::shared_ptr<ff::State> Advance(ff::AppGlobals *globals) override;
	virtual void Render(ff::AppGlobals * globals, ff::IRenderTarget *target, ff::IRenderDepth* depth) override;

private:
	void AddEntities();

	std::unique_ptr<ff::IRenderer> _render;
	ff::Viewport _viewport;
	ff::EntityDomain _domain;
	ff::IEntityBucket<TestPalette::UpdateSystemEntry>* _updateEntityBucket;
	ff::IEntityBucket<TestPalette::RenderSystemEntry>* _renderEntityBucket;
	ff::TypedResource<ff::ISpriteList> _paletteSpritesResource;
	ff::TypedResource<ff::ISpriteFont> _fontResource;
	ff::TypedResource<ff::IPaletteData> _paletteResource;
	ff::ComPtr<ff::IPalette> _palette;
	ff::ComPtr<ff::ITexture> _paletteTextureSmall;
	ff::ComPtr<ff::IRenderTarget> _paletteTargetSmall;
	ff::ComPtr<ff::IRenderDepth> _paletteDepthSmall;
	ff::ComPtr<ff::ITexture> _paletteTextureFull;
	ff::ComPtr<ff::IRenderTarget> _paletteTargetFull;
};
