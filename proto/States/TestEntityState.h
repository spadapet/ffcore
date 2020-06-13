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
}

struct UpdateSystemEntry;
struct RenderSystemEntry;

class TestEntityState : public ff::State
{
public:
	TestEntityState(ff::AppGlobals* globals);

	virtual std::shared_ptr<ff::State> Advance(ff::AppGlobals *globals) override;
	virtual void Render(ff::AppGlobals * globals, ff::IRenderTarget *target, ff::IRenderDepth* depth) override;

private:
	void AddEntities();

	std::unique_ptr<ff::IRenderer> _render;
	ff::Viewport _viewport;
	ff::EntityDomain _domain;
	ff::IEntityBucket<UpdateSystemEntry>* _updateEntityBucket;
	ff::IEntityBucket<RenderSystemEntry>* _renderEntityBucket;
	ff::TypedResource<ff::ISprite> _colorSpriteResource;
	ff::TypedResource<ff::ISpriteList> _paletteSpritesResource;
	ff::TypedResource<ff::ISpriteFont> _fontResource;
	ff::ComPtr<ff::IPalette> _palette;
};
