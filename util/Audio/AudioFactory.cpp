#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioFactory.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"

class __declspec(uuid("b1c52643-fed3-458a-b998-3036c344ced9"))
	AudioFactory
	: public ff::ComBase
	, public ff::IAudioFactory
	, public ff::IXAudioFactory
{
public:
	DECLARE_HEADER(AudioFactory);

	// IAudioFactory
	virtual IXAudio2* GetAudio() override;

	virtual ff::ComPtr<ff::IAudioDevice> CreateDevice() override;
	virtual size_t GetDeviceCount() const override;
	virtual ff::IAudioDevice* GetDevice(size_t nIndex) const override;

	virtual void AddChild(ff::IAudioDevice* child) override;
	virtual void RemoveChild(ff::IAudioDevice* child) override;

	virtual ff::IXAudioFactory* AsXAudioFactory() override;

private:
	ff::Mutex _mutex;
	ff::ComPtr<IXAudio2> _audio;
	ff::Vector<ff::IAudioDevice*> _devices;
	ff::IAudioDevice* _defaultDevice;
};

BEGIN_INTERFACES(AudioFactory)
	HAS_INTERFACE(ff::IAudioFactory)
END_INTERFACES()

bool CreateAudioDevice(ff::IAudioFactory* factory, ff::StringRef name, size_t channels, size_t sampleRate, ff::IAudioDevice** device);

ff::ComPtr<ff::IAudioFactory> ff::CreateAudioFactory()
{
	ff::ComPtr<ff::IAudioFactory> obj;
	assertHrRetVal(ff::ComAllocator<AudioFactory>::CreateInstance(
		nullptr, GUID_NULL, __uuidof(ff::IAudioFactory), (void**)&obj), false);
	return obj;
}

AudioFactory::AudioFactory()
	: _defaultDevice(nullptr)
{
	::MFStartup(MF_VERSION);
}

AudioFactory::~AudioFactory()
{
	assert(_devices.IsEmpty());

	_audio = nullptr;

	::MFShutdown();
}

IXAudio2* AudioFactory::GetAudio()
{
	if (!_audio)
	{
		ff::LockMutex lock(_mutex);

		if (!_audio)
		{
			assertHrRetVal(XAudio2Create(&_audio), nullptr);

			if (ff::GetThisModule().IsDebugBuild())
			{
				XAUDIO2_DEBUG_CONFIGURATION dc;
				ff::ZeroObject(dc);
				dc.TraceMask = XAUDIO2_LOG_ERRORS;
				dc.BreakMask = XAUDIO2_LOG_ERRORS;
				_audio->SetDebugConfiguration(&dc);
			}
		}
	}

	return _audio;
}

ff::ComPtr<ff::IAudioDevice> AudioFactory::CreateDevice()
{
	ff::ComPtr<ff::IAudioDevice> pDevice;
	assertRetVal(::CreateAudioDevice(this, ff::GetEmptyString(), 0, 0, &pDevice), false);

	return pDevice;
}

size_t AudioFactory::GetDeviceCount() const
{
	return _devices.Size();
}

ff::IAudioDevice* AudioFactory::GetDevice(size_t nIndex) const
{
	assertRetVal(nIndex >= 0 && nIndex < _devices.Size(), nullptr);
	return _devices[nIndex];
}

void AudioFactory::AddChild(ff::IAudioDevice* child)
{
	ff::LockMutex lock(_mutex);

	assert(child && _devices.Find(child) == ff::INVALID_SIZE);
	_devices.Push(child);
}

void AudioFactory::RemoveChild(ff::IAudioDevice* child)
{
	ff::LockMutex lock(_mutex);

	verify(_devices.DeleteItem(child));
}

ff::IXAudioFactory* AudioFactory::AsXAudioFactory()
{
	return this;
}
