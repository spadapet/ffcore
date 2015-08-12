#include "pch.h"
#include "Audio/AudioEffect.h"
#include "Audio/AudioMusic.h"
#include "Audio/AudioPlaying.h"
#include "Globals/AppGlobals.h"
#include "Graph/Anim/AnimKeys.h"
#include "Graph/Font/SpriteFont.h"
#include "Graph/GraphDevice.h"
#include "Graph/Render/Renderer.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/RenderTarget/RenderDepth.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Graph/Sprite/Sprite.h"
#include "Graph/Sprite/SpriteAnimation.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Pointer/PointerDevice.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Module/Module.h"
#include "States/TitleState.h"

static ff::hash_t PLAY_EFFECT_EVENT = ff::HashFunc(L"playEffect");
static ff::hash_t PLAY_MUSIC_EVENT = ff::HashFunc(L"playMusic");

TitleState::TitleState(ff::AppGlobals* globals)
	: _initialized(false)
	, _rotate(0)
	, _animFrame(0)
{
	ff::IGraphDevice* graph = ff::AppGlobals::Get()->GetGraph();
	_render = graph->CreateRenderer();
}

TitleState::~TitleState()
{
}

std::shared_ptr<ff::State> TitleState::Advance(ff::AppGlobals* globals)
{
	bool playMusic = false;

	if (!_initialized)
	{
		_initialized = true;
		_sprite.Init(L"TestTexture");
		_anim.Init(L"TestSpriteAnim");
		_effect.Init(L"TestEffect");
		_music.Init(L"TestMusic");
		_font.Init(L"TestFont");

		_inputRes.Init(L"TestCommands");
		_inputDevices._keys.Push(globals->GetKeys());
		_inputDevices._mice.Push(globals->GetPointer());
	}

	_timer.Tick();
	_rotate = std::fmod(_rotate + ff::PI_F / 240, 2.0f * ff::PI_F);
	_input = _inputRes.GetObject();

	if (_input)
	{
		_input->Advance(_inputDevices, _timer.GetTickSeconds());

		for (const ff::InputEvent& event : _input->GetEvents())
		{
			if (event.IsStart())
			{
				if (event._eventID == PLAY_EFFECT_EVENT && _effect.GetObject())
				{
					_effect.GetObject()->Play();
				}
				else if (event._eventID == PLAY_MUSIC_EVENT)
				{
					if (_musicPlaying)
					{
						_musicPlaying->Stop();
						_musicPlaying = nullptr;
					}
					else
					{
						playMusic = true;
					}
				}
			}
		}
	}

	if (playMusic || (_musicPlaying && !_musicPlaying->IsPlaying()))
	{
		bool wasPlaying = _musicPlaying != nullptr;
		_musicPlaying = nullptr;

		if (_music.GetObject() && _music.GetObject()->Play(&_musicPlaying, false))
		{
			if (!wasPlaying)
			{
				_musicPlaying->FadeIn(2);
			}

			_musicPlaying->Resume();
		}
	}

	if (_anim.GetObject())
	{
		double frame = std::fmod(_timer.GetSeconds(), _anim.GetObject()->GetLastFrame());
		_animFrame = (float)frame;
	}

	return nullptr;
}

void TitleState::Render(ff::AppGlobals* globals, ff::IRenderTarget* target, ff::IRenderDepth* depth)
{
	ff::RectFloat view = _viewport.GetView(target);
	ff::IKeyboardDevice* keys = globals->GetKeys();
	ff::IPointerDevice* mouse = globals->GetPointer();
	ff::RendererActive render;

	render = _render->BeginRender(target, depth, view, ff::RectFloat(view.Size()));
	if (render)
	{
		float absCos = std::abs(std::cos(_rotate));
		float absSin = std::abs(std::sin(_rotate));

		render->DrawFilledRectangle(ff::RectFloat(view.Size()), DirectX::XMFLOAT4(0, 0, 0.5, 1));

		DirectX::XMFLOAT4 rectColors[4] =
		{
			DirectX::XMFLOAT4(absCos, absSin, 0, 1),
			DirectX::XMFLOAT4(0, absCos, absSin, 1),
			DirectX::XMFLOAT4(absCos, 0, absSin, 1),
			DirectX::XMFLOAT4(absSin, absCos, 0, 1),
		};

		render->DrawFilledRectangle(ff::RectFloat(50, 50, 200, 200), rectColors);

		rectColors[0].w = 0.25;
		rectColors[1].w = 0.25;
		rectColors[2].w = 0.25;
		rectColors[3].w = 0.25;

		for (float i = 0; i < 100; i += 20)
		{
			render->DrawFilledRectangle(ff::RectFloat(75 + i, 75 + i, 225 + i, 225 + i), rectColors);
		}

		ff::ISprite* sprite = _sprite.GetObject();
		if (sprite)
		{
#if 1
			render->DrawSprite(sprite, ff::PointFloat(500, 300), ff::PointFloat(2, 2), _rotate, ff::GetColorWhite());
#else
			render->PushOpaque();

			for (size_t i = 0; i < 1000; i++)
			{
				float scale = (rand() % 4) + 1.0f;
				render->DrawSprite(
					sprite,
					ff::PointInt(rand() % 1000, rand() % 500).ToType<float>(),
					ff::PointFloat(scale, scale),
					_rotate,
					ff::GetColorWhite());
			}

			render->PopOpaque();
#endif
		}

		if (_font.GetObject())
		{
			ff::PointFloat scale(absCos + 1, absSin + 1);

			_font.GetObject()->DrawText(render,
				ff::String(L"Hello World!\r\nThis is a test"),
				ff::PointFloat(50, 300),
				scale,
				DirectX::XMFLOAT4(1, absCos, absSin, 1));
		}

		if (_anim.GetObject())
		{
			_anim.GetObject()->Render(
				render,
				ff::POSE_TWEEN_LINEAR_CLAMP,
				_animFrame,
				ff::PointFloat(500, 600),
				ff::PointFloat::Ones(),
				0,
				ff::GetColorWhite());

			DirectX::XMFLOAT4 animColor(1, 1, 1, 0.5);

			for (float i = 0; i < 50; i += 10)
			{
				_anim.GetObject()->Render(
					render,
					ff::POSE_TWEEN_LINEAR_CLAMP,
					_animFrame,
					ff::PointFloat(600 + i, 600 - i / 2),
					ff::PointFloat::Ones(),
					0,
					animColor);
			}
		}
	}

	ff::PointFloat swapSize = target->GetRotatedSize().ToType<float>();
	ff::RectFloat viewRect(swapSize);
	const float maxKey = 128;

	// Keyboard indicators
	render = _render->BeginRender(target, nullptr, viewRect, ff::RectFloat(0, 0, maxKey, 1));
	if (render)
	{
		for (float vk = 0; vk < maxKey; vk++)
		{
			if (keys->GetKey((int)vk))
			{
				render->DrawFilledRectangle(ff::RectFloat(vk + 0, 0, vk + 1, 1), DirectX::XMFLOAT4(1 - (vk / maxKey), vk / maxKey, 0, 1));
			}
		}
	}

	// View outlines
	render = _render->BeginRender(target, nullptr, viewRect, viewRect);
	if (render)
	{
		float absSin = std::abs(std::sin(_rotate));
		DirectX::XMFLOAT4 lineColor(1, 1, 1, 0.5);

		ff::RectFloat outerRect = viewRect;
		outerRect.Deflate(1, 1);

		render->DrawOutlineRectangle(outerRect, DirectX::XMFLOAT4(0, 1, 0, 1), absSin * 30 + 2);

		render->DrawLine(
			viewRect.TopLeft() + ff::PointFloat(10, 10),
			viewRect.BottomRight() - ff::PointFloat(10, 10),
			lineColor, absSin * 30 + 2);

		render->DrawLine(
			viewRect.BottomLeft() + ff::PointFloat(10, -10),
			viewRect.TopRight() + ff::PointFloat(-10, 10),
			lineColor, absSin * 30 + 2);

		if (_font.GetObject())
		{
			size_t fps = _timer.GetTicksPerSecond();
			double seconds = _timer.GetSeconds();

			_font.GetObject()->DrawText(render,
				ff::String::format_new(L"View: (%.1f,%.1f)\nSwap: (%.1f,%.1f)\nFPS:%lu, S:%.4f",
					viewRect.Width(), viewRect.Height(), swapSize.x, swapSize.y, fps, seconds),
				ff::PointFloat(10, 10),
				ff::PointFloat(.5, .5),
				DirectX::XMFLOAT4(1, 0, 0, 1));
		}
	}

	// Mouse and touch indicators
	render = _render->BeginRender(target, nullptr, viewRect, viewRect);
	if (render)
	{
		ff::PointFloat mousePos = mouse->GetPos().ToType<float>();

		render->DrawOutlineCircle(viewRect.Center(), mousePos.x - viewRect.Center().x, ff::GetColorMagenta(), 1, true);

		if (mouse->GetButton(VK_LBUTTON))
		{
			render->DrawFilledRectangle(ff::RectFloat(
				mousePos - ff::PointFloat(10, 10),
				mousePos + ff::PointFloat(10, 10)), DirectX::XMFLOAT4(1, 0, 0, 1));
		}
		else
		{
			render->DrawOutlineRectangle(ff::RectFloat(
				mousePos - ff::PointFloat(10, 10),
				mousePos + ff::PointFloat(10, 10)), DirectX::XMFLOAT4(1, 0, 0, 1), 1);
		}

		for (size_t i = 0; i < mouse->GetTouchCount(); i++)
		{
			const ff::TouchInfo& info = mouse->GetTouchInfo(i);
			if (info.type != ff::INPUT_DEVICE_MOUSE)
			{
				ff::PointDouble halfSize = (info.type == ff::INPUT_DEVICE_PEN) ? ff::PointDouble(20, 20) : ff::PointDouble(60, 60);
				DirectX::XMFLOAT4 color = (info.type == ff::INPUT_DEVICE_PEN) ? DirectX::XMFLOAT4(0, 1, 1, 1) : DirectX::XMFLOAT4(0, 1, 0, 1);
				render->DrawFilledRectangle(ff::RectFloat(
					(info.pos - halfSize).ToType<float>(),
					(info.pos + halfSize).ToType<float>()), color);
			}
		}
	}
}
