#pragma once

#include "Audio/AudioDeviceChild.h"

namespace ff
{
	class IAudioPlaying;

	class __declspec(uuid("31361e4d-00ed-4493-a498-2b0627d6de91")) __declspec(novtable)
		IAudioEffect : public IUnknown, public IAudioDeviceChild
	{
	public:
		virtual bool Play(bool startPlaying = true, float volume = 1, float freqRatio = 1, IAudioPlaying** playing = nullptr) = 0;
		virtual bool IsPlaying() const = 0;
		virtual void StopAll() = 0;
	};
}
