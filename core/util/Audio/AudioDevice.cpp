#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioDeviceChild.h"
#include "Audio/AudioEffect.h"
#include "Audio/AudioFactory.h"
#include "Audio/AudioPlaying.h"
#include "COM/ComAlloc.h"
#include "Globals/ThreadGlobals.h"

class __declspec(uuid("039ec1dd-9e1a-4d08-9678-7937a0671d9a"))
	AudioDevice
	: public ff::ComBase
	, public ff::IAudioDevice
	, public ff::IXAudioDevice
{
public:
	DECLARE_HEADER(AudioDevice);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	bool Init(ff::StringRef name, size_t channels, size_t sampleRate);

	// IAudioDevice functions
	virtual bool IsValid() const override;
	virtual void Destroy() override;
	virtual bool Reset() override;

	virtual void Stop() override;
	virtual void Start() override;

	virtual float GetVolume(ff::AudioVoiceType type) const override;
	virtual void SetVolume(ff::AudioVoiceType type, float volume) override;

	virtual void AdvanceEffects() override;
	virtual void StopEffects() override;
	virtual void PauseEffects() override;
	virtual void ResumeEffects() override;

	virtual void AddChild(ff::IAudioDeviceChild* child) override;
	virtual void RemoveChild(ff::IAudioDeviceChild* child) override;
	virtual void AddPlaying(ff::IAudioPlaying* child) override;
	virtual void RemovePlaying(ff::IAudioPlaying* child) override;

	virtual ff::IXAudioDevice* AsXAudioDevice() override;
	virtual IXAudio2* GetAudio() const override;
	virtual IXAudio2Voice* GetVoice(ff::AudioVoiceType type) const override;

private:
	ff::Mutex _mutex;
	ff::ComPtr<ff::IAudioFactory> _factory;
	IXAudio2MasteringVoice* _masterVoice;
	IXAudio2SubmixVoice* _effectVoice;
	IXAudio2SubmixVoice* _musicVoice;
	ff::Vector<ff::IAudioDeviceChild*> _children;
	ff::Vector<ff::IAudioPlaying*> _playing;
	ff::Vector<ff::IAudioPlaying*> _paused;
	ff::String _name;
	size_t _channels;
	size_t _sampleRate;
	size_t _advances;
};

BEGIN_INTERFACES(AudioDevice)
	HAS_INTERFACE(ff::IAudioDevice)
END_INTERFACES()

bool CreateAudioDevice(ff::IAudioFactory* factory, ff::StringRef name, size_t channels, size_t sampleRate, ff::IAudioDevice** device)
{
	assertRetVal(device, false);
	*device = nullptr;

	ff::ComPtr<AudioDevice, ff::IAudioDevice> pDevice;
	assertHrRetVal(ff::ComAllocator<AudioDevice>::CreateInstance(factory, &pDevice), false);
	assertRetVal(pDevice->Init(name, channels, sampleRate), false);

	*device = pDevice.Detach();
	return true;
}

AudioDevice::AudioDevice()
	: _masterVoice(nullptr)
	, _effectVoice(nullptr)
	, _musicVoice(nullptr)
	, _channels(0)
	, _sampleRate(0)
	, _advances(0)
{
}

AudioDevice::~AudioDevice()
{
	assert(!_children.Size());

	Destroy();

	if (_factory)
	{
		_factory->RemoveChild(this);
	}
}

HRESULT AudioDevice::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_factory.QueryFrom(unkOuter), E_INVALIDARG);
	_factory->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool AudioDevice::Init(ff::StringRef name, size_t channels, size_t sampleRate)
{
	assertRetVal(!_masterVoice, false);
	assertRetVal(_factory->AsXAudioFactory() && _factory->AsXAudioFactory()->GetAudio(), false);

	HRESULT hrMaster = _factory->AsXAudioFactory()->GetAudio()
		? _factory->AsXAudioFactory()->GetAudio()->CreateMasteringVoice(
			&_masterVoice,
			(UINT32)channels,
			(UINT32)sampleRate,
			0, // flags
			name.size() ? name.c_str() : nullptr,
			nullptr)
		: E_FAIL;

	if (SUCCEEDED(hrMaster))
	{
		XAUDIO2_SEND_DESCRIPTOR masterDescriptor;
		masterDescriptor.Flags = 0;
		masterDescriptor.pOutputVoice = _masterVoice;

		XAUDIO2_VOICE_SENDS masterSend;
		masterSend.SendCount = 1;
		masterSend.pSends = &masterDescriptor;

		XAUDIO2_VOICE_DETAILS details;
		ff::ZeroObject(details);
		_masterVoice->GetVoiceDetails(&details);

		assertHrRetVal(_factory->AsXAudioFactory()->GetAudio()->CreateSubmixVoice(
			&_effectVoice,
			details.InputChannels,
			details.InputSampleRate,
			0, // flags
			0, // stage
			&masterSend,
			nullptr), false);

		assertHrRetVal(_factory->AsXAudioFactory()->GetAudio()->CreateSubmixVoice(
			&_musicVoice,
			details.InputChannels,
			details.InputSampleRate,
			0, // flags
			0, // stage
			&masterSend,
			nullptr), false);
	}

	_name = name;
	_channels = channels;
	_sampleRate = sampleRate;

	return FAILED(hrMaster) || IsValid();
}

bool AudioDevice::IsValid() const
{
	return _masterVoice != nullptr;
}

void AudioDevice::Destroy()
{
	for (size_t i = 0; i < _children.Size(); i++)
	{
		_children[i]->Reset();
	}

	Stop();

	if (_effectVoice)
	{
		_effectVoice->DestroyVoice();
		_effectVoice = nullptr;
	}

	if (_musicVoice)
	{
		_musicVoice->DestroyVoice();
		_musicVoice = nullptr;
	}

	if (_masterVoice)
	{
		_masterVoice->DestroyVoice();
		_masterVoice = nullptr;
	}
}

bool AudioDevice::Reset()
{
	Destroy();
	assertRetVal(Init(_name, _channels, _sampleRate), false);

	return true;
}

void AudioDevice::Stop()
{
	if (_factory->AsXAudioFactory()->GetAudio())
	{
		StopEffects();
		_factory->AsXAudioFactory()->GetAudio()->StopEngine();
	}
}

void AudioDevice::Start()
{
	if (_factory->AsXAudioFactory()->GetAudio())
	{
		_factory->AsXAudioFactory()->GetAudio()->StartEngine();
	}
}

float AudioDevice::GetVolume(ff::AudioVoiceType type) const
{
	float volume = 1;

	if (GetVoice(type))
	{
		GetVoice(type)->GetVolume(&volume);
	}

	return volume;
}

void AudioDevice::SetVolume(ff::AudioVoiceType type, float volume)
{
	if (GetVoice(type))
	{
		volume = std::max<float>(0, volume);
		volume = std::min<float>(1, volume);

		GetVoice(type)->SetVolume(volume);
	}
}

void AudioDevice::AdvanceEffects()
{
	// Check if speakers were plugged in every two seconds
	if (++_advances % 120 == 0 && !IsValid())
	{
		Reset();
	}

	for (auto i = _playing.crbegin(); i != _playing.crend(); i++)
	{
		// OK to advance paused effects, I guess (change if needed)
		(*i)->Advance();
	}
}

void AudioDevice::StopEffects()
{
	for (size_t i = 0; i < _playing.Size(); i++)
	{
		_playing[i]->Stop();
	}
}

void AudioDevice::PauseEffects()
{
	ff::Vector<ff::IAudioPlaying*> paused;

	for (size_t i = 0; i < _playing.Size(); i++)
	{
		ff::IAudioPlaying* playing = _playing[i];
		if (!playing->IsPaused())
		{
			paused.Push(playing);
			playing->Pause();
		}
	}

	ff::LockMutex lock(_mutex);

	for (ff::IAudioPlaying* playing : paused)
	{
		if (!_paused.Contains(playing))
		{
			_paused.Push(playing);
		}
	}
}

void AudioDevice::ResumeEffects()
{
	ff::LockMutex lock(_mutex);
	ff::Vector<ff::IAudioPlaying*> paused = std::move(_paused);
	lock.Unlock();

	for (ff::IAudioPlaying* playing : paused)
	{
		playing->Resume();
	}
}

ff::IXAudioDevice* AudioDevice::AsXAudioDevice()
{
	return this;
}

IXAudio2* AudioDevice::GetAudio() const
{
	return _factory->AsXAudioFactory()->GetAudio();
}

IXAudio2Voice* AudioDevice::GetVoice(ff::AudioVoiceType type) const
{
	switch (type)
	{
	case ff::AudioVoiceType::MASTER:
		return _masterVoice;

	case ff::AudioVoiceType::EFFECTS:
		return _effectVoice;

	case ff::AudioVoiceType::MUSIC:
		return _musicVoice;

	default:
		assertRetVal(false, _masterVoice);
	}
}

void AudioDevice::AddChild(ff::IAudioDeviceChild* child)
{
	ff::LockMutex lock(_mutex);

	assert(child && _children.Find(child) == ff::INVALID_SIZE);
	_children.Push(child);
}

void AudioDevice::RemoveChild(ff::IAudioDeviceChild* child)
{
	ff::LockMutex lock(_mutex);

	verify(_children.DeleteItem(child));
}

void AudioDevice::AddPlaying(ff::IAudioPlaying* child)
{
	ff::LockMutex lock(_mutex);

	assert(child && _playing.Find(child) == ff::INVALID_SIZE);
	_playing.Push(child);
}

void AudioDevice::RemovePlaying(ff::IAudioPlaying* child)
{
	ff::LockMutex lock(_mutex);

	_paused.DeleteItem(child);
	verify(_playing.DeleteItem(child));
}
