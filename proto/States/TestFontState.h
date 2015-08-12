#pragma once

#include "Resource/ResourceValue.h"
#include "State/State.h"

namespace ff
{
	class IRenderDepth;
	class IRenderer;
	class IResources;
	class ISpriteFont;
}

class TestFontState : public ff::State
{
public:
	TestFontState();
	~TestFontState();

	virtual std::shared_ptr<ff::State> Advance(ff::AppGlobals *context) override;
	virtual void Render(ff::AppGlobals *context, ff::IRenderTarget *target, ff::IRenderDepth* depth) override;

private:
	void CreateSpriteFont();

	bool _createdSpriteFont;
	std::unique_ptr<ff::IRenderer> _render;
	ff::ComPtr<ff::IResources> _resources;
	ff::TypedResource<ff::ISpriteFont> _font;
	ff::TypedResource<ff::ISpriteFont> _font2;
};
