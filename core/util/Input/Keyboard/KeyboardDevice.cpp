#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ThreadGlobals.h"
#include "Input/DeviceEvent.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Module/Module.h"
#include "Windows/WinUtil.h"

#if !METRO_APP

class __declspec(uuid("c62f6c11-e3ef-4e92-8a5e-3c7d92dac943"))
	KeyboardDevice
	: public ff::ComBase
	, public ff::IKeyboardDevice
	, public ff::IDeviceEventProvider
	, public ff::IWindowProcListener
{
public:
	DECLARE_HEADER(KeyboardDevice);

	bool Init(HWND hwnd);
	void Destroy();
	HWND GetWindow() const;

	// IInputDevice
	virtual void Advance() override;
	virtual void KillPending() override;
	virtual bool IsConnected() const override;

	// IKeyboardDevice
	virtual bool GetKey(int vk) const override;
	virtual int GetKeyPressCount(int vk) const override;
	virtual ff::String GetChars() const override;

	// IDeviceEventProvider
	virtual void SetSink(ff::IDeviceEventSink* sink) override;

	// IWindowProcListener
	virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult) override;

private:
	static const size_t KEY_COUNT = 256;

	struct KeyInfo
	{
		BYTE _keys[KEY_COUNT];
		BYTE _presses[KEY_COUNT];
	};

	ff::Mutex _mutex;
	ff::ComPtr<ff::IDeviceEventSink> _sink;
	HWND _hwnd;
	KeyInfo _keyInfo;
	KeyInfo _pendingKeyInfo;
	ff::String _text;
	ff::String _textPending;
};

BEGIN_INTERFACES(KeyboardDevice)
	HAS_INTERFACE(ff::IKeyboardDevice)
	HAS_INTERFACE(ff::IInputDevice)
	HAS_INTERFACE(ff::IDeviceEventProvider)
END_INTERFACES()

bool ff::CreateKeyboardDevice(HWND hwnd, ff::IKeyboardDevice **ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;

	ff::ComPtr<KeyboardDevice, IKeyboardDevice> pInput;
	assertHrRetVal(ff::ComAllocator<KeyboardDevice>::CreateInstance(&pInput), false);
	assertRetVal(pInput->Init(hwnd), false);

	*ppInput = pInput.Detach();
	return true;
}

KeyboardDevice::KeyboardDevice()
	: _hwnd(nullptr)
{
	ff::ZeroObject(_keyInfo);
	ff::ZeroObject(_pendingKeyInfo);
}

KeyboardDevice::~KeyboardDevice()
{
	Destroy();
}

bool KeyboardDevice::Init(HWND hwnd)
{
	assertRetVal(hwnd, false);

	_hwnd = hwnd;
	ff::ThreadGlobals::Get()->AddWindowListener(_hwnd, this);

	return true;
}

void KeyboardDevice::Destroy()
{
	KillPending();

	if (_hwnd)
	{
		ff::ThreadGlobals::Get()->RemoveWindowListener(_hwnd, this);
		_hwnd = nullptr;
	}
}

void KeyboardDevice::Advance()
{
	ff::LockMutex lock(_mutex);

	_text = std::move(_textPending);
	_keyInfo = _pendingKeyInfo;
	ff::ZeroObject(_pendingKeyInfo._presses);
}

void KeyboardDevice::KillPending()
{
	ff::Vector<ff::DeviceEvent, 16> deviceEvents;
	{
		ff::LockMutex lock(_mutex);

		for (unsigned int i = 0; i < _countof(_pendingKeyInfo._keys); i++)
		{
			if (_pendingKeyInfo._keys[i])
			{
				_pendingKeyInfo._keys[i] = false;
				deviceEvents.Push(ff::DeviceEventKeyPress(i, 0));
			}
		}
	}

	if (_sink)
	{
		for (const ff::DeviceEvent& deviceEvent : deviceEvents)
		{
			_sink->AddEvent(deviceEvent);
		}
	}
}

bool KeyboardDevice::IsConnected() const
{
	return true;
}

HWND KeyboardDevice::GetWindow() const
{
	return _hwnd;
}

bool KeyboardDevice::GetKey(int vk) const
{
	assert(vk >= 0 && vk < KEY_COUNT);
	return _keyInfo._keys[vk] != 0;
}

int KeyboardDevice::GetKeyPressCount(int vk) const
{
	assert(vk >= 0 && vk < KEY_COUNT);
	return (int)_keyInfo._presses[vk];
}

ff::String KeyboardDevice::GetChars() const
{
	return _text;
}

void KeyboardDevice::SetSink(ff::IDeviceEventSink* sink)
{
	_sink = sink;
}

bool KeyboardDevice::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult)
{
	switch (msg)
	{
	case WM_DESTROY:
		Destroy();
		break;

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		KillPending();
		break;

	case WM_KEYDOWN:
		if (wParam >= 0 && wParam < KEY_COUNT)
		{
			if (_sink)
			{
				_sink->AddEvent(ff::DeviceEventKeyPress((unsigned int)wParam, (int)(lParam & 0xFFFF)));
			}

			if (!(lParam & 0x40000000)) // wasn't already down
			{
				ff::LockMutex lock(_mutex);

				if (_pendingKeyInfo._presses[wParam] != 0xFF)
				{
					_pendingKeyInfo._presses[wParam]++;
				}

				_pendingKeyInfo._keys[wParam] = true;
			}
		}
		break;

	case WM_KEYUP:
		if (wParam >= 0 && wParam < KEY_COUNT)
		{
			if (_sink)
			{
				_sink->AddEvent(ff::DeviceEventKeyPress((int)wParam, 0));
			}

			ff::LockMutex lock(_mutex);
			_pendingKeyInfo._keys[wParam] = false;
		}
		break;

	case WM_CHAR:
		if (wParam)
		{
			if (_sink)
			{
				_sink->AddEvent(ff::DeviceEventKeyChar((wchar_t)wParam));
			}

			ff::LockMutex lock(_mutex);
			_textPending.append(1, (wchar_t)wParam);
		}
		break;
	}

	return false;
}

#endif // !METRO_APP
