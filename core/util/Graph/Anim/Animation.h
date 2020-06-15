#pragma once

#include "Value/Value.h"
#include "Value/ValueType.h"

namespace ff
{
	class Dict;
	class IAnimation;
	class IAnimationPlayer;
	class IAudioEffect;
	class IRendererActive;
	struct Transform;

	struct AnimationEvent
	{
		IAnimation* _animation;
		IAudioEffect* _effect;
		const Dict* _properties;
		hash_t _event;
	};

	class __declspec(uuid("4a99d6de-38db-416a-ab9d-081aea5913cc")) __declspec(novtable)
		IAnimation : public IUnknown
	{
	public:
		virtual float GetFrameLength() const = 0;
		virtual float GetFramesPerSecond() const = 0;
		virtual void GetFrameEvents(float start, float end, bool includeStart, ItemCollector<AnimationEvent>& events) = 0;
		virtual void RenderFrame(IRendererActive* render, const Transform& position, float frame, const Dict* params = nullptr) = 0;
		virtual ValuePtr GetFrameValue(hash_t name, float frame, const Dict* params = nullptr) = 0;
		virtual ComPtr<IAnimationPlayer> CreateAnimationPlayer(float startFrame = 0, float timeScale = 1, const Dict* params = nullptr) = 0;
	};

	class __declspec(uuid("f737b74c-95df-482c-998c-953a0704e79f")) __declspec(novtable)
		IAnimationPlayer : public IUnknown
	{
	public:
		virtual void AdvanceAnimation(ItemCollector<AnimationEvent>* frameEvents = nullptr) = 0;
		virtual void RenderAnimation(IRendererActive* render, const Transform& position) = 0;
		virtual float GetCurrentFrame() const = 0;
		virtual IAnimation* GetAnimation() = 0;
	};
}
