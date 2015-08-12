#include "pch.h"
#include "Audio/AudioDevice.h"
#include "Audio/AudioDeviceChild.h"
#include "Globals/ProcessGlobals.h"
#include "Module/ModuleFactory.h"
#include "Thread/ThreadPool.h"

class __declspec(uuid("3638c79d-784e-4ded-830e-26e400fa5af5"))
	DestroyVoiceWorkItem
	: public ff::ComBase
	, public ff::IAudioDeviceChild
	, public IUnknown
{
public:
	DECLARE_HEADER(DestroyVoiceWorkItem);

	bool Init(IXAudio2SourceVoice* source);

	virtual HRESULT _Construct(IUnknown* unkOuter) override;
	virtual void _DeleteThis() override;

	// IAudioDeviceChild
	virtual ff::IAudioDevice* GetDevice() const override;
	virtual bool Reset() override;

private:
	ff::Mutex _mutex;
	ff::ComPtr<ff::IAudioDevice> _device;
	IXAudio2SourceVoice* _source;
};

BEGIN_INTERFACES(DestroyVoiceWorkItem)
END_INTERFACES()

// STATIC_DATA (object)
static ff::PoolAllocator<ff::ComObject<DestroyVoiceWorkItem>> s_destroyVoiceAllocator;

static HRESULT CreateDestroyVoiceWorkItemNoInit(IUnknown* unkOuter, REFGUID clsid, REFGUID iid, void** obj)
{
	assertRetVal(clsid == GUID_NULL || clsid == __uuidof(DestroyVoiceWorkItem), E_INVALIDARG);
	ff::ComPtr<ff::ComObject<DestroyVoiceWorkItem>> myObj = s_destroyVoiceAllocator.New();
	assertHrRetVal(myObj->_Construct(unkOuter), E_FAIL);

	return myObj->QueryInterface(iid, obj);
}

static bool CreateDestroyVoiceWorkItem(ff::IAudioDevice* device, IXAudio2SourceVoice* source, DestroyVoiceWorkItem** obj)
{
	assertRetVal(obj, false);

	ff::ComPtr<DestroyVoiceWorkItem> myObj;
	assertHrRetVal(::CreateDestroyVoiceWorkItemNoInit(device, GUID_NULL, __uuidof(DestroyVoiceWorkItem), (void**)&myObj), false);
	assertRetVal(myObj->Init(source), false);

	*obj = myObj.Detach();
	return true;
}

void DestroyVoiceAsync(ff::IAudioDevice* device, IXAudio2SourceVoice* source)
{
	assertRet(device && source);

	ff::ComPtr<DestroyVoiceWorkItem> obj;
	if (!::CreateDestroyVoiceWorkItem(device, source, &obj))
	{
		source->DestroyVoice();
	}
}

DestroyVoiceWorkItem::DestroyVoiceWorkItem()
	: _source(nullptr)
{
}

DestroyVoiceWorkItem::~DestroyVoiceWorkItem()
{
	Reset();

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT DestroyVoiceWorkItem::_Construct(IUnknown* unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_INVALIDARG);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

void DestroyVoiceWorkItem::_DeleteThis()
{
	s_destroyVoiceAllocator.Delete(static_cast<ff::ComObject<DestroyVoiceWorkItem>*>(this));
}

bool DestroyVoiceWorkItem::Init(IXAudio2SourceVoice* source)
{
	assertRetVal(_device && source, false);
	_source = source;

	ff::ComPtr<DestroyVoiceWorkItem> keepAlive = this;
	ff::GetThreadPool()->AddTask([keepAlive]()
		{
			keepAlive->Reset();
		});

	return true;
}

ff::IAudioDevice* DestroyVoiceWorkItem::GetDevice() const
{
	return _device;
}

bool DestroyVoiceWorkItem::Reset()
{
	if (_source)
	{
		ff::LockMutex lock(_mutex);
		if (_source)
		{
			_source->DestroyVoice();
			_source = nullptr;
		}
	}

	return true;
}
