#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioEffect.h"
#include "Audio/AudioPlaying.h"
#include "Audio/AudioStream.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Data/Stream.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/ResourceValue.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Types/Timer.h"
#include "Value/Values.h"
#include "Windows/Handles.h"

#define DEBUG_THIS_FILE 0 // DEBUG

class AudioMusicPlaying;
class AudioSourceReaderCallback;

static ff::StaticString PROP_MP3(L"mp3");
static ff::StaticString PROP_VOLUME(L"volume");
static ff::StaticString PROP_FREQ(L"freq");
static ff::StaticString PROP_LOOP(L"loop");

template<typename T>
static T Clamp(T value, T minValue, T maxValue)
{
	return std::min<T>(std::max<T>(value, minValue), maxValue);
}

class __declspec(uuid("7bd9c9be-b944-4c4c-95ee-218c516d71f8"))
	AudioMusic
	: public ff::ComBase
	, public ff::IAudioEffect
	, public ff::IResourcePersist
{
public:
	DECLARE_HEADER(AudioMusic);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	void OnMusicDone(AudioMusicPlaying* playing);

	// IAudioEffect
	virtual bool Play(bool startPlaying, float volume, float freqRatio, ff::IAudioPlaying** obj) override;
	virtual bool IsPlaying() const override;
	virtual void StopAll() override;

	// IAudioDeviceChild
	virtual ff::IAudioDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

private:
	typedef ff::Vector<ff::ComPtr<AudioMusicPlaying, ff::IAudioPlaying>, 4> PlayingVector;

	PlayingVector _playing;
	ff::ComPtr<ff::IAudioDevice> _device;
	ff::TypedResource<ff::IAudioStream> _streamRes;
	float _volume;
	float _freqRatio;
	bool _loop;
};

class __declspec(uuid("4900158e-fcdd-4179-b85e-63aa12f6ee1f"))
	AudioMusicPlaying
	: public ff::ComBase
	, public ff::IAudioPlaying
	, public IXAudio2VoiceCallback
	, public IMFSourceReaderCallback
{
public:
	DECLARE_HEADER(AudioMusicPlaying);

	bool Init(AudioMusic* parent, ff::IAudioStream* stream, bool startPlaying, float volume, float freqRatio, bool loop);
	void SetParent(AudioMusic* parent);
	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	virtual void _Destruct() override;

	enum class State
	{
		MUSIC_INVALID,
		MUSIC_INIT,
		MUSIC_PLAYING,
		MUSIC_PAUSED,
		MUSIC_DONE
	};

	State GetState() const;

	// IAudioPlaying functions
	virtual bool IsPlaying() const override;
	virtual bool IsPaused() const override;
	virtual bool IsStopped() const override;
	virtual bool IsMusic() const override;

	virtual void Advance() override;
	virtual void Stop() override;
	virtual void Pause() override;
	virtual void Resume() override;

	virtual double GetDuration() const override;
	virtual double GetPosition() const override;
	virtual bool SetPosition(double value) override;
	virtual double GetVolume() const override;
	virtual bool SetVolume(double value) override;
	virtual bool FadeIn(double value) override;
	virtual bool FadeOut(double value) override;

	// IAudioDeviceChild
	virtual ff::IAudioDevice* GetDevice() const override;
	virtual bool Reset() override;

	// IXAudio2VoiceCallback functions
	COM_FUNC_VOID OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
	COM_FUNC_VOID OnVoiceProcessingPassEnd() override;
	COM_FUNC_VOID OnStreamEnd() override;
	COM_FUNC_VOID OnBufferStart(void* pBufferContext) override;
	COM_FUNC_VOID OnBufferEnd(void* pBufferContext) override;
	COM_FUNC_VOID OnLoopEnd(void* pBufferContext) override;
	COM_FUNC_VOID OnVoiceError(void* pBufferContext, HRESULT error) override;

	// IMFSourceReaderCallback
	COM_FUNC OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
	COM_FUNC OnFlush(DWORD dwStreamIndex) override;
	COM_FUNC OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

private:
	bool AsyncInit();
	void StartAsyncWork();
	void RunAsyncWork();
	void CancelAsync();
	void StartReadSample();
	void OnMusicDone();
	void UpdateSourceVolume(IXAudio2SourceVoice* source);

	static const size_t MAX_BUFFERS = 2;

	struct BufferInfo
	{
		ff::Vector<BYTE> _buffer;
		LONGLONG _startTime;
		LONGLONG _duration;
		UINT64 _startSamples;
		DWORD _streamIndex;
		DWORD _streamFlags;
	};

	enum class MediaState
	{
		None,
		Reading,
		Flushing,
		Done,
	};

	ff::Mutex _mutex;
	State _state;
	LONGLONG _duration; // in 100-nanosecond units
	LONGLONG _desiredPosition;
	ff::WinHandle _asyncEvent; // set when there is no async action running
	ff::WinHandle _stopEvent; // set when everything should stop
	ff::List<BufferInfo> _bufferInfos;

	ff::ComPtr<ff::IAudioDevice> _device;
	ff::ComPtr<ff::IAudioStream> _stream;
	ff::ComPtr<IMFSourceReader> _mediaReader;
	ff::ComPtr<AudioSourceReaderCallback, IMFSourceReaderCallback> _mediaCallback;
	IXAudio2SourceVoice* _source;
	AudioMusic* _parent;

	float _freqRatio;
	float _volume;
	float _playVolume;
	float _fadeVolume;
	float _fadeScale;
	ff::Timer _fadeTimer;
	bool _loop;
	bool _startPlaying;
	MediaState _mediaState;
};

class __declspec(uuid("4d0dc5e5-d387-48a1-8f97-8002bc1adf90"))
	AudioSourceReaderCallback : public ff::ComBase, public IMFSourceReaderCallback
{
public:
	DECLARE_HEADER(AudioSourceReaderCallback);

	void SetParent(AudioMusicPlaying* parent);

	// IMFSourceReaderCallback
	COM_FUNC OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
	COM_FUNC OnFlush(DWORD dwStreamIndex) override;
	COM_FUNC OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

private:
	ff::Mutex _mutex;
	AudioMusicPlaying* _parent;
};

void DestroyVoiceAsync(ff::IAudioDevice* device, IXAudio2SourceVoice* source);

BEGIN_INTERFACES(AudioMusic)
	HAS_INTERFACE(ff::IAudioEffect)
	HAS_INTERFACE(ff::IResourcePersist)
END_INTERFACES()

BEGIN_INTERFACES(AudioMusicPlaying)
	HAS_INTERFACE(ff::IAudioPlaying)
	HAS_INTERFACE(IMFSourceReaderCallback)
END_INTERFACES()

BEGIN_INTERFACES(AudioSourceReaderCallback)
	HAS_INTERFACE(IMFSourceReaderCallback)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<AudioMusic>(L"music");
	});

AudioMusic::AudioMusic()
	: _volume(1)
	, _freqRatio(1)
	, _loop(false)
{
}

AudioMusic::~AudioMusic()
{
	Reset();

	for (AudioMusicPlaying* play : _playing)
	{
		play->SetParent(nullptr);
	}

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT AudioMusic::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

void AudioMusic::OnMusicDone(AudioMusicPlaying* playing)
{
	verify(_playing.DeleteItem(playing));
}

bool AudioMusic::Play(bool startPlaying, float volume, float freqRatio, ff::IAudioPlaying** obj)
{
	noAssertRetVal(_device->IsValid() && _streamRes.GetObject(), false);

	ff::ComPtr<AudioMusicPlaying, ff::IAudioPlaying> playing;
	assertHrRetVal(ff::ComAllocator<AudioMusicPlaying>::CreateInstance(_device, &playing), false);
	assertRetVal(playing->Init(this, _streamRes.GetObject(), startPlaying, _volume * volume, _freqRatio * freqRatio, _loop), false);
	_playing.Push(playing);

	if (obj)
	{
		*obj = playing.Detach();
	}

	return true;
}

bool AudioMusic::IsPlaying() const
{
	for (AudioMusicPlaying* play : _playing)
	{
		if (play->IsPlaying())
		{
			return true;
		}
	}

	return false;
}

void AudioMusic::StopAll()
{
	PlayingVector playing = _playing;

	for (AudioMusicPlaying* play : playing)
	{
		play->Stop();
	}
}

ff::IAudioDevice* AudioMusic::GetDevice() const
{
	return _device;
}

bool AudioMusic::Reset()
{
	return true;
}

bool AudioMusic::LoadFromSource(const ff::Dict& dict)
{
	return LoadFromCache(dict);
}

bool AudioMusic::LoadFromCache(const ff::Dict& dict)
{
	_streamRes.Init(dict.Get<ff::SharedResourceWrapperValue>(PROP_MP3));
	_volume = dict.Get<ff::FloatValue>(PROP_VOLUME, 1.0f);
	_freqRatio = dict.Get<ff::FloatValue>(PROP_FREQ, 1.0f);
	_loop = dict.Get<ff::BoolValue>(PROP_LOOP);

	return true;
}

bool AudioMusic::SaveToCache(ff::Dict& dict)
{
	dict.Set<ff::SharedResourceWrapperValue>(PROP_MP3, _streamRes.GetResourceValue());
	dict.Set<ff::FloatValue>(PROP_VOLUME, _volume);
	dict.Set<ff::FloatValue>(PROP_FREQ, _freqRatio);
	dict.Set<ff::BoolValue>(PROP_LOOP, _loop);

	return true;
}

AudioMusicPlaying::AudioMusicPlaying()
	: _state(State::MUSIC_INVALID)
	, _duration(0)
	, _desiredPosition(0)
	, _parent(nullptr)
	, _source(nullptr)
	, _freqRatio(1)
	, _volume(1)
	, _playVolume(1)
	, _fadeVolume(1)
	, _fadeScale(0)
	, _loop(false)
	, _startPlaying(false)
	, _mediaState(MediaState::None)
{
	_asyncEvent = ff::CreateEvent(true);
	_stopEvent = ff::CreateEvent();

	_mediaCallback = new ff::ComObject<AudioSourceReaderCallback>();
	_mediaCallback->SetParent(this);
}

AudioMusicPlaying::~AudioMusicPlaying()
{
	if (_device)
	{
		_device->RemovePlaying(this);
		_device->RemoveChild(this);
	}
}

HRESULT AudioMusicPlaying::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);
	_device->AddPlaying(this);

	return __super::_Construct(unkOuter);
}

void AudioMusicPlaying::_Destruct()
{
	Reset();

	if (_mediaCallback)
	{
		_mediaCallback->SetParent(nullptr);
	}

	__super::_Destruct();
}

AudioMusicPlaying::State AudioMusicPlaying::GetState() const
{
	return _state;
}

bool AudioMusicPlaying::Init(AudioMusic* parent, ff::IAudioStream* stream, bool startPlaying, float volume, float freqRatio, bool loop)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());
	assertRetVal(_state == State::MUSIC_INVALID, false);
	assertRetVal(stream && _device && _device->AsXAudioDevice()->GetAudio(), false);

	_state = State::MUSIC_INIT;
	_parent = parent;
	_stream = stream;
	_startPlaying = startPlaying;
	_volume = volume;
	_freqRatio = freqRatio;
	_loop = loop;

	StartAsyncWork();

	return true;
}

// background thread init
void AudioMusicPlaying::RunAsyncWork()
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread() && !ff::IsEventSet(_asyncEvent));

	if (!_source && !IsEventSet(_stopEvent))
	{
		verify(AsyncInit());
	}

	StartReadSample();

	::SetEvent(_asyncEvent);
}

bool AudioMusicPlaying::AsyncInit()
{
	assertRetVal(_stream && _mediaCallback && !_source && !_mediaReader, false);

	ff::ComPtr<ff::IDataReader> reader;
	assertRetVal(_stream->CreateReader(&reader), false);

	ff::ComPtr<IMFByteStream> mediaByteStream;
	assertRetVal(ff::CreateReadStream(reader, _stream->GetMimeType(), &mediaByteStream), false);

	ff::ComPtr<IMFSourceResolver> sourceResolver;
	assertHrRetVal(::MFCreateSourceResolver(&sourceResolver), false);

	ff::ComPtr<IUnknown> mediaSourceUnknown;
	MF_OBJECT_TYPE mediaSourceObjectType = MF_OBJECT_MEDIASOURCE;
	DWORD mediaSourceFlags = MF_RESOLUTION_MEDIASOURCE | MF_RESOLUTION_READ | MF_RESOLUTION_DISABLE_LOCAL_PLUGINS;
	assertHrRetVal(sourceResolver->CreateObjectFromByteStream(mediaByteStream, nullptr, mediaSourceFlags, nullptr, &mediaSourceObjectType, &mediaSourceUnknown), false);

	ff::ComPtr<IMFMediaSource> mediaSource;
	assertRetVal(mediaSource.QueryFrom(mediaSourceUnknown), false);

	ff::ComPtr<IMFAttributes> mediaAttributes;
	assertHrRetVal(::MFCreateAttributes(&mediaAttributes, 1), false);
	assertHrRetVal(mediaAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, _mediaCallback.Interface()), false);

	ff::ComPtr<IMFSourceReader> mediaReader;
	assertHrRetVal(::MFCreateSourceReaderFromMediaSource(mediaSource, mediaAttributes, &mediaReader), false);
	assertHrRetVal(mediaReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, false), false);
	assertHrRetVal(mediaReader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, true), false);

	ff::ComPtr<IMFMediaType> mediaType;
	assertHrRetVal(::MFCreateMediaType(&mediaType), false);
	assertHrRetVal(mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio), false);
	assertHrRetVal(mediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float), false);
	assertHrRetVal(mediaReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, mediaType), false);

	ff::ComPtr<IMFMediaType> actualMediaType;
	assertHrRetVal(mediaReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualMediaType), false);

	WAVEFORMATEX* waveFormat = nullptr;
	UINT waveFormatLength = 0;
	assertHrRetVal(::MFCreateWaveFormatExFromMFMediaType(actualMediaType, &waveFormat, &waveFormatLength), false);

	PROPVARIANT durationValue;
	assertHrRetVal(mediaReader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &durationValue), false);

	XAUDIO2_SEND_DESCRIPTOR sendDesc{ 0 };
	sendDesc.pOutputVoice = _device->AsXAudioDevice()->GetVoice(ff::AudioVoiceType::MUSIC);

	XAUDIO2_VOICE_SENDS sends;
	sends.SendCount = 1;
	sends.pSends = &sendDesc;

	IXAudio2SourceVoice* source = nullptr;
	HRESULT hr = _device->AsXAudioDevice()->GetAudio()->CreateSourceVoice(
		&source,
		waveFormat,
		0, // flags
		XAUDIO2_DEFAULT_FREQ_RATIO,
		this, // callback,
		&sends);

	::CoTaskMemFree(waveFormat);
	waveFormat = nullptr;

	if (FAILED(hr))
	{
		if (source)
		{
			source->DestroyVoice();
		}

		assertHrRetVal(hr, false);
	}

	ff::LockMutex lock(_mutex);

	_duration = (LONGLONG)durationValue.uhVal.QuadPart;

	if (_desiredPosition >= 0 && _desiredPosition <= _duration)
	{
		PROPVARIANT value;
		::PropVariantInit(&value);

		value.vt = VT_I8;
		value.hVal.QuadPart = _desiredPosition;
		verifyHr(mediaReader->SetCurrentPosition(GUID_NULL, value));

		::PropVariantClear(&value);
	}

	if (_state == State::MUSIC_INIT)
	{
		UpdateSourceVolume(source);
		source->SetFrequencyRatio(_freqRatio);
		_source = source;
		_mediaReader = mediaReader;
	}
	else
	{
		source->DestroyVoice();
	}

	assertRetVal(_source, false);
	return true;
}

void AudioMusicPlaying::SetParent(AudioMusic* parent)
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);
	_parent = parent;
}

bool AudioMusicPlaying::IsPlaying() const
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);
	return _state == State::MUSIC_PLAYING || (_state == State::MUSIC_INIT && _startPlaying);
}

bool AudioMusicPlaying::IsPaused() const
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);
	return _state == AudioMusicPlaying::State::MUSIC_PAUSED || (_state == State::MUSIC_INIT && !_startPlaying);
}

bool AudioMusicPlaying::IsStopped() const
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);
	return _state == State::MUSIC_DONE;
}

bool AudioMusicPlaying::IsMusic() const
{
	return true;
}

void AudioMusicPlaying::Advance()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	if (_state == State::MUSIC_PLAYING && _source && _fadeScale != 0)
	{
		// Fade the volume in or out
		_fadeTimer.Tick();

		float absFadeScale = std::abs(_fadeScale);
		_fadeVolume = (float)::Clamp(_fadeTimer.GetSeconds() * absFadeScale, 0.0, 1.0);

		bool fadeDone = (_fadeVolume >= 1);
		bool fadeOut = (_fadeScale < 0);

		if (_fadeScale < 0)
		{
			_fadeVolume = 1.0f - _fadeVolume;
		}

		UpdateSourceVolume(_source);

		if (fadeDone)
		{
			_fadeScale = 0;

			if (fadeOut)
			{
				Stop();
			}
		}
	}

	if (_state == State::MUSIC_DONE)
	{
		ff::ComPtr<ff::IAudioPlaying> keepAlive = this;
		ff::LockMutex lock(_mutex);

		if (_state == State::MUSIC_DONE)
		{
			OnMusicDone();
		}
	}
}

void AudioMusicPlaying::Stop()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);

	if (_state != State::MUSIC_DONE)
	{
		if (_source)
		{
			_source->Stop();
		}

		_state = State::MUSIC_DONE;
	}
}

void AudioMusicPlaying::Pause()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);

	if (_state == State::MUSIC_INIT)
	{
		_startPlaying = false;
	}
	else if (_state == State::MUSIC_PLAYING && _source)
	{
		_desiredPosition = (LONGLONG)(GetPosition() * 10000000.0);
		_source->Stop();
		_state = State::MUSIC_PAUSED;
	}
}

void AudioMusicPlaying::Resume()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);

	if (_state == State::MUSIC_INIT)
	{
		_startPlaying = true;
	}
	else if (_state == State::MUSIC_PAUSED && _source)
	{
		_source->Start();
		_state = State::MUSIC_PLAYING;
	}
}

double AudioMusicPlaying::GetDuration() const
{
	ff::LockMutex lock(_mutex);

	return _duration / 10000000.0;
}

double AudioMusicPlaying::GetPosition() const
{
	ff::LockMutex lock(_mutex);
	double pos = 0;
	bool useDesiredPosition = false;

	switch (_state)
	{
	case State::MUSIC_INIT:
	case State::MUSIC_PAUSED:
		useDesiredPosition = true;
		break;

	case State::MUSIC_PLAYING:
	{
		XAUDIO2_VOICE_STATE state;
		_source->GetState(&state);

		XAUDIO2_VOICE_DETAILS details;
		_source->GetVoiceDetails(&details);

		BufferInfo* info = (BufferInfo*)state.pCurrentBufferContext;
		if (info && info->_startSamples != (UINT64)-1)
		{
			double sampleSeconds = (state.SamplesPlayed > info->_startSamples)
				? (state.SamplesPlayed - info->_startSamples) / (double)details.InputSampleRate
				: 0.0;
			pos = sampleSeconds + (info->_startTime / 10000000.0);
		}
		else
		{
			useDesiredPosition = true;
		}
	}
	break;
	}

	if (useDesiredPosition)
	{
		pos = _desiredPosition / 10000000.0;
	}

	return pos;
}

bool AudioMusicPlaying::SetPosition(double value)
{
	ff::LockMutex lock(_mutex);

	assertRetVal(_state != State::MUSIC_DONE, false);

	_desiredPosition = (LONGLONG)(value * 10000000.0);

	if (_mediaReader && _mediaState != MediaState::Flushing)
	{
		_mediaState = MediaState::Flushing;

		if (FAILED(_mediaReader->Flush(MF_SOURCE_READER_FIRST_AUDIO_STREAM)))
		{
			_mediaState = MediaState::None;
			assertRetVal(false, false);
		}
	}

	return true;
}

double AudioMusicPlaying::GetVolume() const
{
	return _playVolume;
}

bool AudioMusicPlaying::SetVolume(double value)
{
	_playVolume = ::Clamp((float)value, 0.0f, 1.0f);
	UpdateSourceVolume(_source);
	return true;
}

bool AudioMusicPlaying::FadeIn(double value)
{
	assertRetVal(!IsPlaying() && value > 0, false);

	_fadeTimer.Reset();
	_fadeScale = (float)(1.0 / ::Clamp(value, 0.0, 10.0));
	_fadeVolume = 0;

	UpdateSourceVolume(_source);

	return true;
}

bool AudioMusicPlaying::FadeOut(double value)
{
	assertRetVal(IsPlaying() && value > 0, false);

	_fadeTimer.Reset();
	_fadeScale = (float)(-1.0 / ::Clamp(value, 0.0, 10.0));
	_fadeVolume = 1;

	UpdateSourceVolume(_source);

	return true;
}

ff::IAudioDevice* AudioMusicPlaying::GetDevice() const
{
	return _device;
}

bool AudioMusicPlaying::Reset()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	CancelAsync();

	ff::LockMutex lock(_mutex);

	if (_source)
	{
		IXAudio2SourceVoice* source = _source;
		_source = nullptr;
		source->DestroyVoice();
	}

	_state = State::MUSIC_DONE;
	return true;
}

void AudioMusicPlaying::OnVoiceProcessingPassStart(UINT32 BytesRequired)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());
}

void AudioMusicPlaying::OnVoiceProcessingPassEnd()
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());
}

void AudioMusicPlaying::OnStreamEnd()
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);
	_state = State::MUSIC_DONE;

	// Loop?
}

void AudioMusicPlaying::OnBufferStart(void* pBufferContext)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);

	if (_bufferInfos.Size() == MAX_BUFFERS)
	{
		// Reuse buffers
		assert(_bufferInfos.GetLast() == pBufferContext);
		_bufferInfos.MoveToBack(*_bufferInfos.GetFirst());
	}

	if (!_bufferInfos.IsEmpty())
	{
		XAUDIO2_VOICE_STATE state;
		_source->GetState(&state);

		BufferInfo* info = _bufferInfos.GetFirst();
		assert(state.pCurrentBufferContext == info);
		info->_startSamples = state.SamplesPlayed;
	}

	StartReadSample();
}

void AudioMusicPlaying::OnBufferEnd(void* pBufferContext)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());
}

void AudioMusicPlaying::OnLoopEnd(void* pBufferContext)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());
}

void AudioMusicPlaying::OnVoiceError(void* pBufferContext, HRESULT error)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());

	assertSz(false, L"XAudio2 voice error");
}

HRESULT AudioMusicPlaying::OnReadSample(
	HRESULT hrStatus,
	DWORD dwStreamIndex,
	DWORD dwStreamFlags,
	LONGLONG llTimestamp,
	IMFSample* pSample)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);

	noAssertRetVal(_mediaState == MediaState::Reading, S_OK);
	bool endOfStream = (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) != 0;
	_mediaState = endOfStream ? MediaState::Done : MediaState::None;

	if (_source && _state != State::MUSIC_DONE)
	{
		ff::ComPtr<IMFMediaBuffer> mediaBuffer;
		BufferInfo* bufferInfo = nullptr;
		BYTE* data = nullptr;
		DWORD dataSize = 0;

		if (pSample && SUCCEEDED(hrStatus) &&
			SUCCEEDED(pSample->ConvertToContiguousBuffer(&mediaBuffer)) &&
			SUCCEEDED(mediaBuffer->Lock(&data, nullptr, &dataSize)))
		{
			if (_bufferInfos.Size() < MAX_BUFFERS)
			{
				_bufferInfos.Push(BufferInfo());
			}

			bufferInfo = _bufferInfos.GetLast();
			bufferInfo->_buffer.Resize(dataSize);
			bufferInfo->_startTime = llTimestamp;
			bufferInfo->_startSamples = (UINT64)-1;
			bufferInfo->_streamIndex = dwStreamIndex;
			bufferInfo->_streamFlags = dwStreamFlags;

			if (FAILED(pSample->GetSampleDuration(&bufferInfo->_duration)))
			{
				bufferInfo->_duration = 0;
			}

			std::memcpy(bufferInfo->_buffer.Data(), data, dataSize);

			mediaBuffer->Unlock();
		}

		if (bufferInfo)
		{
			XAUDIO2_BUFFER buffer;
			ff::ZeroObject(buffer);
			buffer.AudioBytes = (UINT)bufferInfo->_buffer.Size();
			buffer.pAudioData = bufferInfo->_buffer.Data();
			buffer.pContext = bufferInfo;
			buffer.Flags = endOfStream ? XAUDIO2_END_OF_STREAM : 0;
			verifyHr(_source->SubmitSourceBuffer(&buffer));
#if DEBUG_THIS_FILE
			ff::Log::DebugTraceF(L"AudioMusicPlaying::OnReadSample, Size:%lu, Time:%lu-%lu\n",
				bufferInfo->_buffer.Size(),
				llTimestamp,
				llTimestamp + bufferInfo->_duration);
#endif
			if (_state == State::MUSIC_INIT)
			{
				_state = State::MUSIC_PAUSED;

				if (_startPlaying)
				{
					_startPlaying = false;
					_source->Start();
					_state = State::MUSIC_PLAYING;
				}
			}
		}
		else if (_state == State::MUSIC_INIT)
		{
			_state = State::MUSIC_DONE;
		}
		else
		{
			_source->Discontinuity();
		}
	}

	return S_OK;
}

HRESULT AudioMusicPlaying::OnFlush(DWORD dwStreamIndex)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);

	assert(_mediaState == MediaState::Flushing);
	_mediaState = MediaState::None;

#if DEBUG_THIS_FILE
	ff::Log::DebugTraceF(L"AudioMusicPlaying::OnFlush\n");
#endif

	if (_desiredPosition >= 0 && _desiredPosition <= _duration)
	{
		if (_source)
		{
			_startPlaying = _startPlaying || (_state == State::MUSIC_PLAYING);
			_source->Stop();
			_source->FlushSourceBuffers();
			_state = State::MUSIC_INIT;
		}

		PROPVARIANT value;
		::PropVariantInit(&value);
		value.vt = VT_I8;
		value.hVal.QuadPart = _desiredPosition;

		verifyHr(_mediaReader->SetCurrentPosition(GUID_NULL, value));

		::PropVariantClear(&value);
	}

	StartReadSample();

	return S_OK;
}

HRESULT AudioMusicPlaying::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());

	return S_OK;
}

void AudioMusicPlaying::StartAsyncWork()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	::ResetEvent(_asyncEvent);
	ff::ComPtr<AudioMusicPlaying, ff::IAudioPlaying> keepAlive = this;
	ff::GetThreadPool()->AddTask([keepAlive]()
		{
			keepAlive->RunAsyncWork();
		});
}

void AudioMusicPlaying::CancelAsync()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	::SetEvent(_stopEvent);
	ff::WaitForHandle(_asyncEvent);
	::ResetEvent(_stopEvent);
}

void AudioMusicPlaying::StartReadSample()
{
	assert(!ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::LockMutex lock(_mutex);
	HRESULT hr = E_FAIL;

	noAssertRet(_mediaState == MediaState::None);

	if (_mediaReader)
	{
		_mediaState = MediaState::Reading;

		hr = _mediaReader->ReadSample(
			MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			0, nullptr, nullptr, nullptr, nullptr);
	}

	if (FAILED(hr) && _source)
	{
		_source->Discontinuity();
	}
}

void AudioMusicPlaying::OnMusicDone()
{
	assert(ff::GetGameThreadDispatch()->IsCurrentThread());

	ff::ComPtr<IAudioPlaying> keepAlive = this;
	ff::LockMutex lock(_mutex);

	if (_parent)
	{
		_parent->OnMusicDone(this);
		_parent = nullptr;
	}

	if (_source)
	{
		::DestroyVoiceAsync(_device, _source);
		_source = nullptr;
	}
}

void AudioMusicPlaying::UpdateSourceVolume(IXAudio2SourceVoice* source)
{
	assert(source == _source || !ff::GetGameThreadDispatch()->IsCurrentThread());

	if (source)
	{
		source->SetVolume(_volume * _playVolume * _fadeVolume);
	}
}

AudioSourceReaderCallback::AudioSourceReaderCallback()
	: _parent(nullptr)
{
}

AudioSourceReaderCallback::~AudioSourceReaderCallback()
{
	assert(!_parent);
}

void AudioSourceReaderCallback::SetParent(AudioMusicPlaying* parent)
{
	ff::LockMutex lock(_mutex);
	_parent = parent;
}

HRESULT AudioSourceReaderCallback::OnReadSample(
	HRESULT hrStatus,
	DWORD dwStreamIndex,
	DWORD dwStreamFlags,
	LONGLONG llTimestamp,
	IMFSample* pSample)
{
	ff::LockMutex lock(_mutex);

	if (_parent)
	{
		ff::ComPtr<IUnknown> keepAlive = _parent->_GetUnknown();
		return _parent->OnReadSample(hrStatus, dwStreamIndex, dwStreamFlags, llTimestamp, pSample);
	}

	return S_OK;
}

HRESULT AudioSourceReaderCallback::OnFlush(DWORD dwStreamIndex)
{
	ff::LockMutex lock(_mutex);

	if (_parent)
	{
		ff::ComPtr<IUnknown> keepAlive = _parent->_GetUnknown();
		return _parent->OnFlush(dwStreamIndex);
	}

	return S_OK;
}

HRESULT AudioSourceReaderCallback::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
{
	ff::LockMutex lock(_mutex);

	if (_parent)
	{
		ff::ComPtr<IUnknown> keepAlive = _parent->_GetUnknown();
		return _parent->OnEvent(dwStreamIndex, pEvent);
	}

	return S_OK;
}
