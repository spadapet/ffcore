#pragma once

#include "Graph/RenderTarget/Viewport.h"
#include "Input/InputMapping.h"
#include "Resource/ResourceValue.h"
#include "State/State.h"
#include "Types/Timer.h"

namespace ff
{
	class IAnimation;
	class IAnimationPlayer;
	class IAudioEffect;
	class IAudioPlaying;
	class IRenderDepth;
	class IRenderer;
	class ISprite;
	class ISpriteFont;
}

class TitleState : public ff::State
{
public:
	TitleState(ff::AppGlobals* globals);
	~TitleState();

	virtual std::shared_ptr<ff::State> Advance(ff::AppGlobals* globals) override;
	virtual void Render(ff::AppGlobals* globals, ff::IRenderTarget* target, ff::IRenderDepth* depth) override;

private:
	bool _initialized;
	float _rotate;
	ff::TypedResource<ff::ISprite> _sprite;
	ff::TypedResource<ff::IAnimation> _anim;
	ff::TypedResource<ff::ISpriteFont> _font;
	ff::TypedResource<ff::IAudioEffect> _effect;
	ff::TypedResource<ff::IAudioEffect> _music;
	ff::TypedResource<ff::IInputMapping> _inputRes;
	ff::ComPtr<ff::IAnimationPlayer> _animPlayer;
	ff::ComPtr<ff::IAudioPlaying> _musicPlaying;
	std::unique_ptr<ff::IRenderer> _render;
	ff::Viewport _viewport;
	ff::Timer _timer;
	ff::InputDevices _inputDevices;
};
