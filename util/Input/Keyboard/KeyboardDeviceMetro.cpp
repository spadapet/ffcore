#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/DeviceEvent.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Module/Module.h"

#if METRO_APP

class __declspec(uuid("12896528-8ff4-491c-98fd-527c1640e578"))
	KeyboardDevice
	: public ff::ComBase
	, public ff::IKeyboardDevice
	, public ff::IDeviceEventProvider
{
public:
	DECLARE_HEADER(KeyboardDevice);

	bool Init(Windows::UI::Xaml::Window^ window);

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

private:
	ref class KeyEvents
	{
	internal:
		KeyEvents(KeyboardDevice* pParent);

	public:
		virtual ~KeyEvents();

		void Destroy();

	private:
		void OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args);
		void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
		void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);

		KeyboardDevice* _parent;
		Windows::Foundation::EventRegistrationToken _tokens[3];
	};

	void OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args);
	void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);
	void OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);

	static const size_t KEY_COUNT = 256;

	struct KeyInfo
	{
		BYTE _keys[KEY_COUNT];
		BYTE _presses[KEY_COUNT];
	};

	ff::Mutex _mutex;
	ff::ComPtr<ff::IDeviceEventSink> _sink;
	Windows::UI::Xaml::Window^ _window;
	KeyEvents^ _events;
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

KeyboardDevice::KeyEvents::KeyEvents(KeyboardDevice* pParent)
	: _parent(pParent)
{
	_tokens[0] = _parent->_window->CoreWindow->CharacterReceived +=
		ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::CharacterReceivedEventArgs^>(
			this, &KeyEvents::OnCharacterReceived);

	_tokens[1] = _parent->_window->CoreWindow->KeyDown +=
		ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::KeyEventArgs^>(
			this, &KeyEvents::OnKeyDown);

	_tokens[2] = _parent->_window->CoreWindow->KeyUp +=
		ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::KeyEventArgs^>(
			this, &KeyEvents::OnKeyUp);
}

KeyboardDevice::KeyEvents::~KeyEvents()
{
	Destroy();
}

void KeyboardDevice::KeyEvents::Destroy()
{
	if (_parent)
	{
		_parent->_window->CoreWindow->CharacterReceived -= _tokens[0];
		_parent->_window->CoreWindow->KeyDown -= _tokens[1];
		_parent->_window->CoreWindow->KeyUp -= _tokens[2];
		_parent = nullptr;
	}
}

void KeyboardDevice::KeyEvents::OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args)
{
	assertRet(_parent);
	_parent->OnCharacterReceived(sender, args);
}

void KeyboardDevice::KeyEvents::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	assertRet(_parent);
	_parent->OnKeyDown(sender, args);
}

void KeyboardDevice::KeyEvents::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	assertRet(_parent);
	_parent->OnKeyUp(sender, args);
}

// static
bool ff::CreateKeyboardDevice(Windows::UI::Xaml::Window^ window, IKeyboardDevice** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ComPtr<KeyboardDevice, IKeyboardDevice> myObj;
	assertHrRetVal(ComAllocator<KeyboardDevice>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(window), false);

	*obj = myObj.Detach();
	return true;
}

KeyboardDevice::KeyboardDevice()
{
	ff::ZeroObject(_keyInfo);
	ff::ZeroObject(_pendingKeyInfo);
}

KeyboardDevice::~KeyboardDevice()
{
	if (_events)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

bool KeyboardDevice::Init(Windows::UI::Xaml::Window^ window)
{
	assertRetVal(window, false);
	_window = window;
	_events = ref new KeyEvents(this);

	return true;
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

void KeyboardDevice::OnCharacterReceived(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CharacterReceivedEventArgs^ args)
{
	wchar_t ch[3] = { 0 };

	if (args->KeyCode < 0x10000)
	{
		ch[0] = (wchar_t)args->KeyCode;
	}
	else
	{
		unsigned int utf32 = args->KeyCode - 0x10000;
		ch[0] = (wchar_t)((utf32 / 0x400) + 0xd800);
		ch[1] = (wchar_t)((utf32 % 0x400) + 0xdc00);
	}

	if (_sink)
	{
		_sink->AddEvent(ff::DeviceEventKeyChar(args->KeyCode));
	}

	ff::LockMutex lock(_mutex);
	_textPending += ch;
}

void KeyboardDevice::OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	unsigned int vk = (unsigned int)args->VirtualKey;
	noAssertRet(vk >= 0 && vk < KEY_COUNT);

	if (_sink)
	{
		_sink->AddEvent(ff::DeviceEventKeyPress(vk, args->KeyStatus.RepeatCount));
	}

	if (!args->KeyStatus.WasKeyDown)
	{
		ff::LockMutex lock(_mutex);

		if (_pendingKeyInfo._presses[vk] != 0xFF)
		{
			_pendingKeyInfo._presses[vk]++;
		}

		_pendingKeyInfo._keys[vk] = true;
	}
}

void KeyboardDevice::OnKeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	unsigned int vk = (unsigned int)args->VirtualKey;
	noAssertRet(vk >= 0 && vk < KEY_COUNT);

	if (_sink)
	{
		_sink->AddEvent(ff::DeviceEventKeyPress(vk, 0));
	}

	ff::LockMutex lock(_mutex);
	_pendingKeyInfo._keys[vk] = false;
}

#endif
