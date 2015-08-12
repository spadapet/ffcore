#pragma once

#include "Entity/EntityDomain.h"
#include "Graph/RenderTarget/Viewport.h"
#include "Graph/Sprite/Sprite.h"
#include "Resource/ResourceValue.h"
#include "State/State.h"

namespace ff
{
	class IRenderDepth;
	class IRenderer;
	class ISpriteFont;
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
	ff::TypedResource<ff::ISprite> _spriteResource;
	ff::TypedResource<ff::ISpriteFont> _fontResource;
};
