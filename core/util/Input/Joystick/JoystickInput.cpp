#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/DeviceEvent.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"

#if !METRO_APP

class __declspec(uuid("a7955767-043c-410b-9ce1-b60e56ccaa1f"))
	JoystickInput
	: public ff::ComBase
	, public ff::IJoystickInput
	, public ff::IDeviceEventProvider
{
public:
	DECLARE_HEADER(JoystickInput);

	bool Init(HWND hwnd);

	// IInputDevice
	virtual void Advance() override;
	virtual void KillPending() override;
	virtual bool IsConnected() const override;

	// IJoystickInput
	virtual size_t GetCount() const override;
	virtual ff::IJoystickDevice* GetJoystick(size_t nJoy) const override;

	// IDeviceEventProvider
	virtual void SetSink(ff::IDeviceEventSink* sink) override;

private:
	ff::Vector<ff::ComPtr<ff::IJoystickDevice>, 4> _joysticks;
};

BEGIN_INTERFACES(JoystickInput)
	HAS_INTERFACE(ff::IJoystickInput)
	HAS_INTERFACE(ff::IInputDevice)
	HAS_INTERFACE(ff::IDeviceEventProvider)
END_INTERFACES()

bool ff::CreateJoystickInput(HWND hwnd, ff::IJoystickInput** ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;

	ComPtr<JoystickInput, ff::IJoystickInput> pInput;
	assertRetVal(SUCCEEDED(ff::ComAllocator<JoystickInput>::CreateInstance(&pInput)), false);
	assertRetVal(pInput->Init(hwnd), false);

	*ppInput = pInput.Detach();
	return true;
}

JoystickInput::JoystickInput()
{
}

JoystickInput::~JoystickInput()
{
}

bool JoystickInput::Init(HWND hwnd)
{
	assertRetVal(hwnd, false);

	for (DWORD i = 0; i < ff::GetMaxXboxJoystickCount(); i++)
	{
		XINPUT_CAPABILITIES caps;
		ff::ZeroObject(caps);
		DWORD result = ::XInputGetCapabilities(i, 0, &caps);

		if (result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED)
		{
			ff::ComPtr<ff::IJoystickDevice> pDevice;
			assertRetVal(ff::CreateXboxJoystick(hwnd, i, &pDevice), false);
			_joysticks.Push(pDevice);
		}
	}

	return true;
}

void JoystickInput::Advance()
{
	for (ff::IJoystickDevice* device : _joysticks)
	{
		device->Advance();
	}
}

void JoystickInput::KillPending()
{
	for (ff::IJoystickDevice* device : _joysticks)
	{
		device->KillPending();
	}
}

bool JoystickInput::IsConnected() const
{
	for (ff::IJoystickDevice* device : _joysticks)
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


ff::IJoystickDevice* JoystickInput::GetJoystick(size_t nJoy) const
{
	assertRetVal(nJoy >= 0 && nJoy < _joysticks.Size(), nullptr);
	return _joysticks[nJoy];
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

#endif
