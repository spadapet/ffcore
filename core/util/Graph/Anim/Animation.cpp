#include "pch.h"
#include "Audio/AudioEffect.h"
#include "COM/ComObject.h"
#include "Dict/Dict.h"
#include "Graph/Anim/Animation.h"
#include "Graph/Anim/KeyFrames.h"
#include "Graph/Anim/Transform.h"
#include "Graph/Render/MatrixStack.h"
#include "Graph/Render/RendererActive.h"
#include "Graph/Sprite/Sprite.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Value/Values.h"

static ff::StaticString PROP_COLOR(L"color");
static ff::StaticString PROP_EFFECT(L"effect");
static ff::StaticString PROP_EVENT(L"event");
static ff::StaticString PROP_EVENTS(L"events");
static ff::StaticString PROP_FPS(L"fps");
static ff::StaticString PROP_FRAME(L"frame");
static ff::StaticString PROP_KEYS(L"keys");
static ff::StaticString PROP_LENGTH(L"length");
static ff::StaticString PROP_LOOP(L"loop");
static ff::StaticString PROP_METHOD(L"method");
static ff::StaticString PROP_POSITION(L"position");
static ff::StaticString PROP_PROPERTIES(L"properties");
static ff::StaticString PROP_ROTATE(L"rotate");
static ff::StaticString PROP_SCALE(L"scale");
static ff::StaticString PROP_SPEED(L"speed");
static ff::StaticString PROP_START(L"start");
static ff::StaticString PROP_VISUAL(L"visual");
static ff::StaticString PROP_VISUALS(L"visuals");

class __declspec(uuid("e8354436-8bc9-46a6-b469-214b3edf595e"))
	AnimationPlayer
	: public ff::ComBase
	, public ff::IAnimationPlayer
{
public:
	DECLARE_HEADER(AnimationPlayer);

	bool Init(ff::IAnimation* animation, float startFrame, float speed, const ff::Dict* params);

	// IAnimationPlayer
	virtual void AdvanceAnimation(ff::ItemCollector<ff::AnimationEvent>* frameEvents) override;
	virtual void RenderAnimation(ff::IRendererActive* render, const ff::Transform& position) override;
	virtual float GetCurrentFrame() const override;
	virtual ff::IAnimation* GetAnimation() override;

private:
	ff::Dict _params;
	ff::ComPtr<ff::IAnimation> _animation;
	float _start;
	float _frame;
	float _fps;
	float _advances;
};

BEGIN_INTERFACES(AnimationPlayer)
	HAS_INTERFACE(ff::IAnimationPlayer)
END_INTERFACES()

AnimationPlayer::AnimationPlayer()
	: _start(0)
	, _frame(0)
	, _fps(0)
	, _advances(0)
{
}

AnimationPlayer::~AnimationPlayer()
{
}

bool AnimationPlayer::Init(ff::IAnimation* animation, float startFrame, float speed, const ff::Dict* params)
{
	assertRetVal(speed > 0.0, false);

	_start = startFrame;
	_frame = startFrame;
	_fps = speed * animation->GetFramesPerSecond();
	_animation = animation;

	if (params)
	{
		_params = *params;
	}

	return true;
}

void AnimationPlayer::AdvanceAnimation(ff::ItemCollector<ff::AnimationEvent>* frameEvents)
{
	bool firstAdvance = !_advances;
	float beginFrame = _frame;
	_advances += 1.0f;
	_frame = _start + (_advances * _fps / 60.0f);

	if (frameEvents)
	{
		_animation->GetFrameEvents(beginFrame, _frame, firstAdvance, *frameEvents);
	}
}

void AnimationPlayer::RenderAnimation(ff::IRendererActive* render, const ff::Transform& position)
{
	_animation->RenderFrame(render, position, _frame, !_params.IsEmpty() ? &_params : nullptr);
}

float AnimationPlayer::GetCurrentFrame() const
{
	return _frame;
}

ff::IAnimation* AnimationPlayer::GetAnimation()
{
	return _animation;
}

class __declspec(uuid("26dc0ecf-87c0-4b57-b465-bb5520e0bb43"))
	Animation
	: public ff::ComBase
	, public ff::IAnimation
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(Animation);

	// IAnimation
	virtual float GetFrameLength() const override;
	virtual float GetFramesPerSecond() const override;
	virtual void GetFrameEvents(float start, float end, bool includeStart, ff::ItemCollector<ff::AnimationEvent>& events) override;
	virtual void RenderFrame(ff::IRendererActive* render, const ff::Transform& position, float frame, const ff::Dict* params) override;
	virtual ff::ValuePtr GetFrameValue(ff::hash_t name, float frame, const ff::Dict* params) override;
	virtual ff::ComPtr<ff::IAnimationPlayer> CreateAnimationPlayer(float startFrame, float speed, const ff::Dict* params) override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	struct VisualInfo
	{
		bool operator<(const VisualInfo& rhs) const { return _start < rhs._start; }

		float _start;
		float _length;
		float _speed;
		ff::KeyFrames::MethodType _method;
		ff::KeyFrames* _visualKeys;
		ff::KeyFrames* _colorKeys;
		ff::KeyFrames* _positionKeys;
		ff::KeyFrames* _scaleKeys;
		ff::KeyFrames* _rotateKeys;
	};

	struct EventInfo
	{
		bool operator<(const EventInfo& rhs) const { return _frame < rhs._frame; }

		float _frame;
		ff::String _event;
		ff::TypedResource<ff::IAudioEffect> _effect;
		ff::Dict _properties;
		ff::AnimationEvent _publicEvent;
	};

	ff::Vector<ff::ValuePtr> SaveEventsToCache();
	ff::Vector<ff::ValuePtr> SaveVisualsToCache();
	ff::Dict SaveKeysToCache();
	bool Load(const ff::Dict& dict, bool fromCache);
	bool LoadEvents(const ff::Vector<ff::ValuePtr>& values, bool fromCache);
	bool LoadVisuals(const ff::Vector<ff::ValuePtr>& values, bool fromCache);
	bool LoadKeys(const ff::Dict& values, bool fromCache);

	typedef ff::Vector<ff::ComPtr<ff::IAnimation>, 4> CachedVisuals;
	const CachedVisuals* GetCachedVisuals(const ff::ValuePtr& value);

	float _length;
	float _fps;
	ff::KeyFrames::MethodType _method;
	ff::Vector<VisualInfo> _visuals;
	ff::Vector<EventInfo> _events;
	ff::Map<ff::hash_t, ff::KeyFrames, ff::NonHasher<ff::hash_t>> _keys;
	mutable ff::Map<ff::ValuePtr, CachedVisuals> _cachedVisuals;
};

BEGIN_INTERFACES(Animation)
	HAS_INTERFACE(ff::IAnimation)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<Animation>(L"anim");
	});

Animation::Animation()
	: _length(0)
	, _fps(60)
	, _method(ff::KeyFrames::MethodType::None)
{
}

Animation::~Animation()
{
}

void Animation::RenderFrame(ff::IRendererActive* render, const ff::Transform& position, float frame, const ff::Dict* params)
{
	if (!ff::KeyFrames::AdjustFrame(frame, 0.0f, _length, _method))
	{
		return;
	}

	bool pushTransform = (position._rotation != 0);
	const ff::Transform& renderTransform = pushTransform ? ff::Transform::Identity() : position;

	if (pushTransform)
	{
		render->GetWorldMatrixStack().PushMatrix();

		DirectX::XMFLOAT4X4 matrix;
		DirectX::XMStoreFloat4x4(&matrix, position.GetMatrix());
		render->GetWorldMatrixStack().TransformMatrix(matrix);
	}

	for (const VisualInfo& info : _visuals)
	{
		float visualFrame = frame - info._start;
		if (!ff::KeyFrames::AdjustFrame(visualFrame, 0.0f, info._length, info._method))
		{
			continue;
		}

		const CachedVisuals* visuals = GetCachedVisuals(info._visualKeys ? info._visualKeys->GetValue(visualFrame, params) : nullptr);
		if (!visuals || visuals->IsEmpty())
		{
			continue;
		}

		ff::Transform visualTransform = renderTransform;

		if (info._positionKeys)
		{
			ff::ValuePtrT<ff::PointFloatValue> value = info._positionKeys->GetValue(visualFrame, params);
			if (value)
			{
				visualTransform._position += value.GetValue() * renderTransform._scale;
			}
		}

		if (info._scaleKeys)
		{
			ff::ValuePtrT<ff::PointFloatValue> value = info._scaleKeys->GetValue(visualFrame, params);
			if (value)
			{
				visualTransform._scale *= value.GetValue();
			}
		}

		if (info._rotateKeys)
		{
			ff::ValuePtrT<ff::FloatValue> value = info._rotateKeys->GetValue(visualFrame, params);
			if (value)
			{
				visualTransform._rotation += value.GetValue();
			}
		}

		if (info._colorKeys)
		{
			ff::ValuePtrT<ff::RectFloatValue> value = info._colorKeys->GetValue(visualFrame, params);
			if (value)
			{
				DirectX::XMStoreFloat4(&visualTransform._color,
					DirectX::XMVectorMultiply(
						DirectX::XMLoadFloat4(&visualTransform._color),
						DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & value.GetValue())));
			}
		}

		for (const ff::ComPtr<ff::IAnimation>& animVisual : *visuals)
		{
			float visualAnimFrame = (_fps != 0.0f) ? visualFrame * animVisual->GetFramesPerSecond() / _fps : 0.0f;
			animVisual->RenderFrame(render, visualTransform, visualAnimFrame, params);
		}
	}

	if (pushTransform)
	{
		render->GetWorldMatrixStack().PopMatrix();
	}
}

float Animation::GetFrameLength() const
{
	return _length;
}

float Animation::GetFramesPerSecond() const
{
	return _fps;
}

ff::ValuePtr Animation::GetFrameValue(ff::hash_t name, float frame, const ff::Dict* params)
{
	auto i = _keys.GetKey(name);
	return i ? i->GetEditableValue().GetValue(frame, params) : nullptr;
}

void Animation::GetFrameEvents(float start, float end, bool includeStart, ff::ItemCollector<ff::AnimationEvent>& events)
{
	noAssertRet(end > start && _length);

	bool loop = ff::HasAllFlags(_method, ff::KeyFrames::MethodType::BoundsLoop);
	if (loop)
	{
		float length = end - start;
		start = std::fmodf(start, _length);
		end = start + length;
	}

	const EventInfo& findInfo = *reinterpret_cast<const EventInfo*>(&start);
	for (auto i = std::lower_bound(_events.cbegin(), _events.cend(), findInfo); i != _events.cend(); i++)
	{
		const EventInfo& info = *i;
		if (info._frame > end)
		{
			break;
		}
		else if (includeStart || info._frame != start)
		{
			events.Push(info._publicEvent);
		}
	}

	if (loop && end > _length && start > 0)
	{
		float loopEnd = std::min(end - _length, start);
		GetFrameEvents(0, loopEnd, true, events);
	}
}

ff::ComPtr<ff::IAnimationPlayer> Animation::CreateAnimationPlayer(float startFrame, float speed, const ff::Dict* params)
{
	ff::ComPtr<AnimationPlayer, ff::IAnimationPlayer> player = new ff::ComObject<AnimationPlayer>();
	assertRetVal(player->Init(this, startFrame, speed, params), nullptr);

	ff::ComPtr<ff::IAnimationPlayer> result;
	result.Attach(player.DetachInterface());
	return result;
}

bool Animation::LoadFromSource(const ff::Dict& dict)
{
	return Load(dict, false);
}

bool Animation::LoadFromCache(const ff::Dict& dict)
{
	return Load(dict, true);
}

bool Animation::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::FloatValue>(::PROP_LENGTH, _length);
	dict.Set<ff::FloatValue>(::PROP_FPS, _fps);
	dict.Set<ff::IntValue>(::PROP_METHOD, (int)_method);

	dict.Set<ff::ValueVectorValue>(::PROP_EVENTS, SaveEventsToCache());
	dict.Set<ff::ValueVectorValue>(::PROP_VISUALS, SaveVisualsToCache());
	dict.Set<ff::DictValue>(::PROP_KEYS, SaveKeysToCache());

	return true;
}

ff::Vector<ff::ValuePtr> Animation::SaveEventsToCache()
{
	ff::Vector<ff::ValuePtr> values;

	for (EventInfo& event : _events)
	{
		ff::Dict dict;

		dict.Set<ff::FloatValue>(::PROP_FRAME, event._frame);
		dict.Set<ff::StringValue>(::PROP_EVENT, event._event);
		dict.Set<ff::DictValue>(::PROP_PROPERTIES, ff::Dict(event._properties));

		ff::SharedResourceValue effectRes = event._effect.GetResourceValue();
		ff::ComPtr<ff::IAudioEffect> effect = event._effect.Flush();

		if (effectRes && effectRes->GetName().size() && effectRes->GetValue() && !effectRes->GetValue()->IsType<ff::NullValue>())
		{
			dict.Set<ff::SharedResourceWrapperValue>(::PROP_EFFECT, effectRes);
		}
		else if (effect)
		{
			dict.Set<ff::ObjectValue>(::PROP_EFFECT, effect);
		}

		values.Push(ff::Value::New<ff::DictValue>(std::move(dict)));
	}

	return values;
}

ff::Vector<ff::ValuePtr> Animation::SaveVisualsToCache()
{
	ff::Vector<ff::ValuePtr> values;

	for (const VisualInfo& i : _visuals)
	{
		ff::Dict dict;

		dict.Set<ff::FloatValue>(::PROP_START, i._start);
		dict.Set<ff::FloatValue>(::PROP_LENGTH, i._length);
		dict.Set<ff::FloatValue>(::PROP_SPEED, i._speed);
		dict.Set<ff::IntValue>(::PROP_METHOD, (int)i._method);

		if (i._visualKeys)
		{
			dict.Set<ff::StringValue>(::PROP_VISUAL, i._visualKeys->GetName());
		}

		if (i._colorKeys)
		{
			dict.Set<ff::StringValue>(::PROP_COLOR, i._colorKeys->GetName());
		}

		if (i._positionKeys)
		{
			dict.Set<ff::StringValue>(::PROP_POSITION, i._positionKeys->GetName());
		}

		if (i._scaleKeys)
		{
			dict.Set<ff::StringValue>(::PROP_SCALE, i._scaleKeys->GetName());
		}

		if (i._rotateKeys)
		{
			dict.Set<ff::StringValue>(::PROP_ROTATE, i._rotateKeys->GetName());
		}

		values.Push(ff::Value::New<ff::DictValue>(std::move(dict)));
	}

	return values;
}

ff::Dict Animation::SaveKeysToCache()
{
	ff::Dict keys;

	for (auto& i : _keys)
	{
		const ff::KeyFrames& key = i.GetValue();
		keys.Set<ff::DictValue>(key.GetName(), key.SaveToCache());
	}

	return keys;
}

bool Animation::Load(const ff::Dict& dict, bool fromCache)
{
	_length = dict.Get<ff::FloatValue>(::PROP_LENGTH);
	_fps = dict.Get<ff::FloatValue>(::PROP_FPS);
	_method = ff::KeyFrames::LoadMethod(dict, fromCache);

	assertRetVal(LoadKeys(dict.Get<ff::DictValue>(::PROP_KEYS), fromCache), false);
	assertRetVal(LoadVisuals(dict.Get<ff::ValueVectorValue>(::PROP_VISUALS), fromCache), false);
	assertRetVal(LoadEvents(dict.Get<ff::ValueVectorValue>(::PROP_EVENTS), fromCache), false);

	return true;
}

bool Animation::LoadEvents(const ff::Vector<ff::ValuePtr>& values, bool fromCache)
{
	for (ff::ValuePtr value : values)
	{
		ff::ValuePtrT<ff::DictValue> dictValue = value;
		assertRetVal(dictValue, false);

		const ff::Dict& dict = dictValue.GetValue();
		EventInfo event;
		event._frame = dict.Get<ff::FloatValue>(::PROP_FRAME);
		event._event = dict.Get<ff::StringValue>(::PROP_EVENT);
		event._properties = dict.Get<ff::DictValue>(::PROP_PROPERTIES);

		ff::ValuePtrT<ff::SharedResourceWrapperValue> effectRes = dict.GetValue(::PROP_EFFECT);
		if (effectRes)
		{
			event._effect.Init(effectRes.GetValue());
		}
		else
		{
			ff::ValuePtrT<ff::ObjectValue> effectObject = dict.GetValue(::PROP_EFFECT);
			ff::ComPtr<ff::IAudioEffect> effect;

			if (effect.QueryFrom(effectObject.GetValue()))
			{
				event._effect.Init(std::make_shared<ff::ResourceValue>(effect, ff::GetEmptyString()));
			}
		}

		_events.Push(std::move(event));
	}

	for (EventInfo& event : _events)
	{
		event._publicEvent._animation = this;
		event._publicEvent._effect = event._effect.Flush();
		event._publicEvent._event = ff::HashFunc(event._event);
		event._publicEvent._properties = &event._properties;
	}

	return true;
}

bool Animation::LoadVisuals(const ff::Vector<ff::ValuePtr>& values, bool fromCache)
{
	for (ff::ValuePtr value : values)
	{
		ff::ValuePtrT<ff::DictValue> dictValue = value;
		assertRetVal(dictValue, false);

		const ff::Dict& dict = dictValue.GetValue();
		VisualInfo info{};
		info._start = dict.Get<ff::FloatValue>(::PROP_START);
		info._length = dict.Get<ff::FloatValue>(::PROP_LENGTH, std::max(_length - info._start, 0.0f));
		info._speed = dict.Get<ff::FloatValue>(::PROP_SPEED, 1.0f);
		info._method = ff::KeyFrames::LoadMethod(dict, fromCache);

		ff::String visualKeys = dict.Get<ff::StringValue>(::PROP_VISUAL);
		ff::String colorKeys = dict.Get<ff::StringValue>(::PROP_COLOR);
		ff::String positionKeys = dict.Get<ff::StringValue>(::PROP_POSITION);
		ff::String scaleKeys = dict.Get<ff::StringValue>(::PROP_SCALE);
		ff::String rotateKeys = dict.Get<ff::StringValue>(::PROP_ROTATE);

		auto visualIter = visualKeys.size() ? _keys.GetKey(ff::HashFunc(visualKeys)) : nullptr;
		auto colorIter = colorKeys.size() ? _keys.GetKey(ff::HashFunc(colorKeys)) : nullptr;
		auto positionIter = positionKeys.size() ? _keys.GetKey(ff::HashFunc(positionKeys)) : nullptr;
		auto scaleIter = scaleKeys.size() ? _keys.GetKey(ff::HashFunc(scaleKeys)) : nullptr;
		auto rotateIter = rotateKeys.size() ? _keys.GetKey(ff::HashFunc(rotateKeys)) : nullptr;

		assertRetVal(visualKeys.empty() || visualIter, false);
		assertRetVal(colorKeys.empty() || colorIter, false);
		assertRetVal(positionKeys.empty() || positionIter, false);
		assertRetVal(scaleKeys.empty() || scaleIter, false);
		assertRetVal(rotateKeys.empty() || rotateIter, false);

		info._visualKeys = visualIter ? &visualIter->GetEditableValue() : nullptr;
		info._colorKeys = colorIter ? &colorIter->GetEditableValue() : nullptr;
		info._positionKeys = positionIter ? &positionIter->GetEditableValue() : nullptr;
		info._scaleKeys = scaleIter ? &scaleIter->GetEditableValue() : nullptr;
		info._rotateKeys = rotateIter ? &rotateIter->GetEditableValue() : nullptr;

		_visuals.Push(std::move(info));
	}

	return true;
}

bool Animation::LoadKeys(const ff::Dict& values, bool fromCache)
{
	for (ff::StringRef name : values.GetAllNames())
	{
		ff::Dict dict = values.Get<ff::DictValue>(name);
		_keys.SetKey(ff::HashFunc(name), fromCache ? ff::KeyFrames::LoadFromCache(dict) : ff::KeyFrames::LoadFromSource(name, dict));
	}

	return true;
}

const Animation::CachedVisuals* Animation::GetCachedVisuals(const ff::ValuePtr& value)
{
	noAssertRetVal(value, nullptr);

	auto i = _cachedVisuals.GetKey(value);
	if (!i)
	{
		CachedVisuals visuals;

		ff::ComPtr<ff::IAnimation> anim;
		if (anim.QueryFrom(value->GetComObject()))
		{
			visuals.Push(std::move(anim));
		}
		else if (value->IsType<ff::ValueVectorValue>())
		{
			visuals.Reserve(value->GetValue<ff::ValueVectorValue>().Size());

			for (ff::ValuePtr childValue : value->GetValue<ff::ValueVectorValue>())
			{
				ff::ComPtr<ff::IAnimation> anim;
				if (anim.QueryFrom(childValue->GetComObject()))
				{
					visuals.Push(anim);
				}
				else
				{
					assertSz(false, L"Not a valid animation visual");
				}
			}
		}
		else
		{
			assertSzRetVal(false, L"Not a valid animation visual", nullptr);
		}

		i = _cachedVisuals.SetKey(ff::ValuePtr(value), std::move(visuals));
	}

	return &i->GetValue();
}

ff::CreateAnimation::CreateAnimation(float length, float fps, KeyFrames::MethodType method)
{
	_dict.Set<ff::FloatValue>(::PROP_LENGTH, length);
	_dict.Set<ff::FloatValue>(::PROP_FPS, fps);
	_dict.Set<ff::IntValue>(::PROP_METHOD, (int)method);
}

ff::CreateAnimation::CreateAnimation(const CreateAnimation& rhs)
	: _dict(rhs._dict)
	, _events(rhs._events)
	, _keys(rhs._keys)
	, _visuals(rhs._visuals)
{
}

ff::CreateAnimation::CreateAnimation(CreateAnimation&& rhs)
	: _dict(std::move(rhs._dict))
	, _events(std::move(rhs._events))
	, _keys(std::move(rhs._keys))
	, _visuals(std::move(rhs._visuals))
{
}

void ff::CreateAnimation::AddKeys(const CreateKeyFrames& key)
{
	ff::String name;
	ff::Dict dict = key.CreateSourceDict(name);
	_keys.Set<ff::DictValue>(name, std::move(dict));
}

void ff::CreateAnimation::AddEvent(float frame, StringRef name, IAudioEffect* effect, const Dict* properties)
{
	ff::Dict dict;
	dict.Set<ff::FloatValue>(::PROP_FRAME, frame);
	dict.Set<ff::StringValue>(::PROP_EVENT, name);

	if (effect)
	{
		dict.Set<ff::ObjectValue>(::PROP_EFFECT, effect);
	}

	if (properties)
	{
		dict.Set<ff::DictValue>(::PROP_PROPERTIES, ff::Dict(*properties));
	}

	_events.Push(ff::Value::New<ff::DictValue>(std::move(dict)));
}

void ff::CreateAnimation::AddVisual(
	float start,
	float length,
	float speed,
	KeyFrames::MethodType method,
	StringRef visualKeys,
	StringRef colorKeys,
	StringRef positionKeys,
	StringRef scaleKeys,
	StringRef rotateKeys)
{
	ff::Dict dict;
	dict.Set<ff::FloatValue>(::PROP_START, start);
	dict.Set<ff::FloatValue>(::PROP_LENGTH, length);
	dict.Set<ff::FloatValue>(::PROP_SPEED, speed);
	dict.Set<ff::IntValue>(::PROP_METHOD, (int)method);
	dict.Set<ff::StringValue>(::PROP_VISUAL, visualKeys);
	dict.Set<ff::StringValue>(::PROP_COLOR, colorKeys);
	dict.Set<ff::StringValue>(::PROP_POSITION, positionKeys);
	dict.Set<ff::StringValue>(::PROP_SCALE, scaleKeys);
	dict.Set<ff::StringValue>(::PROP_ROTATE, rotateKeys);

	_visuals.Push(ff::Value::New<ff::DictValue>(std::move(dict)));
}

ff::ComPtr<ff::IAnimation> ff::CreateAnimation::Create() const
{
	ff::Dict dict = _dict;
	dict.Set<ff::DictValue>(::PROP_KEYS, ff::Dict(_keys));
	dict.Set<ff::ValueVectorValue>(::PROP_VISUALS, ff::Vector<ff::ValuePtr>(_visuals));
	dict.Set<ff::ValueVectorValue>(::PROP_EVENTS, ff::Vector<ff::ValuePtr>(_events));

	ff::ComPtr<Animation, ff::IAnimation> anim = new ff::ComObject<Animation>();
	assertRetVal(anim->LoadFromSource(dict), nullptr);

	ff::ComPtr<ff::IAnimation> result;
	result.Attach(anim.DetachInterface());
	return result;
}
