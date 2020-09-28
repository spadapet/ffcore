#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/DeviceEvent.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"

#if METRO_APP

class __declspec(uuid("3292c799-eb7a-4337-b2c2-87861ec1a03f"))
	JoystickInput
	: public ff::ComBase
	, public ff::IJoystickInput
	, public ff::IDeviceEventProvider
{
public:
	DECLARE_HEADER(JoystickInput);

	bool Init();

	// IInputDevice
	virtual void Advance() override;
	virtual void KillPending() override;
	virtual bool IsConnected() const override;

	// IJoystickInput
	virtual size_t GetCount() const override;
	virtual ff::IJoystickDevice* GetJoystick(size_t index) const override;

	// IDeviceEventProvider
	virtual void SetSink(ff::IDeviceEventSink * sink) override;

private:
	ref class JoyEvents
	{
	internal:
		JoyEvents(JoystickInput* parent);

	public:
		virtual ~JoyEvents();

		void Destroy();

	private:
		void OnGamepadAdded(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad);
		void OnGamepadRemoved(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad);

		JoystickInput* _parent;
		Windows::Foundation::EventRegistrationToken _tokens[2];
	};

	void OnGamepadAdded(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad);
	void OnGamepadRemoved(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad);

	JoyEvents^ _events;
	ff::Vector<ff::ComPtr<ff::IXboxJoystick>> _joysticks;
};

BEGIN_INTERFACES(JoystickInput)
	HAS_INTERFACE(ff::IJoystickInput)
	HAS_INTERFACE(ff::IDeviceEventProvider)
END_INTERFACES()

bool ff::CreateJoystickInput(IJoystickInput** obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ComPtr<JoystickInput, ff::IJoystickInput> pInput;
	assertHrRetVal(ComAllocator<JoystickInput>::CreateInstance(&pInput), false);
	assertRetVal(pInput->Init(), false);

	*obj = pInput.Detach();
	return true;
}

JoystickInput::JoyEvents::JoyEvents(JoystickInput* parent)
	: _parent(parent)
{
	_tokens[0] = Windows::Gaming::Input::Gamepad::GamepadAdded +=
		ref new Windows::Foundation::EventHandler<Windows::Gaming::Input::Gamepad^>(
			this, &JoyEvents::OnGamepadAdded);

	_tokens[1] = Windows::Gaming::Input::Gamepad::GamepadRemoved +=
		ref new Windows::Foundation::EventHandler<Windows::Gaming::Input::Gamepad^>(
			this, &JoyEvents::OnGamepadRemoved);
}

JoystickInput::JoyEvents::~JoyEvents()
{
	Destroy();
}

void JoystickInput::JoyEvents::Destroy()
{
	if (_parent)
	{
		Windows::Gaming::Input::Gamepad::GamepadAdded -= _tokens[0];
		Windows::Gaming::Input::Gamepad::GamepadRemoved -= _tokens[1];
		_parent = nullptr;
	}
}

void JoystickInput::JoyEvents::OnGamepadAdded(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad)
{
	if (_parent)
	{
		_parent->OnGamepadAdded(sender, gamepad);
	}
}

void JoystickInput::JoyEvents::OnGamepadRemoved(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad)
{
	if (_parent)
	{
		_parent->OnGamepadRemoved(sender, gamepad);
	}
}

JoystickInput::JoystickInput()
{
}

JoystickInput::~JoystickInput()
{
	if (_events)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

bool JoystickInput::Init()
{
	_events = ref new JoyEvents(this);

	for (size_t i = 0; i < 4; i++)
	{
		ff::ComPtr<ff::IXboxJoystick> device;
		assertRetVal(ff::CreateXboxJoystick(nullptr, &device), false);
		_joysticks.Push(device);
	}

	for (Windows::Gaming::Input::Gamepad^ gamepad : Windows::Gaming::Input::Gamepad::Gamepads)
	{
		OnGamepadAdded(nullptr, gamepad);
	}

	return true;
}

void JoystickInput::Advance()
{
	for (ff::IXboxJoystick* device : _joysticks)
	{
		device->Advance();
	}
}

void JoystickInput::KillPending()
{
	for (ff::IXboxJoystick* device : _joysticks)
	{
		device->KillPending();
	}
}

bool JoystickInput::IsConnected() const
{
	for (ff::IXboxJoystick* device : _joysticks)
	{
		if (device->IsConnected())
		{
			return true;
		}
	}

	return false;
}

size_t JoystickInput::GetCount() const
{
	return _joysticks.Size();
}

ff::IJoystickDevice* JoystickInput::GetJoystick(size_t index) const
{
	assertRetVal(index >= 0 && index < _joysticks.Size(), nullptr);
	return _joysticks[index];
}

void JoystickInput::SetSink(ff::IDeviceEventSink* sink)
{
	for (size_t i = 0; i < GetCount(); i++)
	{
		ff::ComPtr<ff::IDeviceEventProvider> provider;
		if (provider.QueryFrom(GetJoystick(i)))
		{
			provider->SetSink(sink);
		}
	}
}

void JoystickInput::OnGamepadAdded(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad)
{
	for (size_t i = 0; i < _joysticks.Size(); i++)
	{
		if (!_joysticks[i]->GetGamepad())
		{
			_joysticks[i]->SetGamepad(gamepad);
			gamepad = nullptr;
			break;
		}
	}

	if (gamepad)
	{
		ff::ComPtr<ff::IXboxJoystick> device;
		assertRet(ff::CreateXboxJoystick(gamepad, &device));
		_joysticks.Push(device);
	}
}

void JoystickInput::OnGamepadRemoved(Platform::Object^ sender, Windows::Gaming::Input::Gamepad^ gamepad)
{
	for (size_t i = 0; i < _joysticks.Size(); i++)
	{
		if (_joysticks[i]->GetGamepad() == gamepad)
		{
			_joysticks[i]->SetGamepad(nullptr);
			break;
		}
	}
}

#endif
