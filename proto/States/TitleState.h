#pragma once

#include "Graph/RenderTarget/Viewport.h"
#include "Input/InputMapping.h"
#include "Resource/ResourceValue.h"
#include "State/State.h"
#include "Types/Timer.h"

namespace ff
{
	class IAudioEffect;
	class IAudioMusic;
	class IAudioPlaying;
	class IRenderDepth;
	class IRenderer;
	class ISprite;
	class ISpriteAnimation;
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
	float _animFrame;
	ff::TypedResource<ff::ISprite> _sprite;
	ff::TypedResource<ff::ISpriteAnimation> _anim;
	ff::TypedResource<ff::ISpriteFont> _font;
	ff::TypedResource<ff::IAudioEffect> _effect;
	ff::TypedResource<ff::IAudioMusic> _music;
	ff::TypedResource<ff::IInputMapping> _inputRes;
	ff::ComPtr<ff::IInputMapping> _input;
	ff::ComPtr<ff::IAudioPlaying> _musicPlaying;
	std::unique_ptr<ff::IRenderer> _render;
	ff::Viewport _viewport;
	ff::Timer _timer;
	ff::InputDevices _inputDevices;
};
