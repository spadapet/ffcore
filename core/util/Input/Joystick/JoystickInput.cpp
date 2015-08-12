#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"

#if !METRO_APP

class __declspec(uuid("a7955767-043c-410b-9ce1-b60e56ccaa1f"))
	JoystickInput : public ff::ComBase, public ff::IJoystickInput
{
public:
	DECLARE_HEADER(JoystickInput);

	bool Init();

	// IJoystickInput
	virtual void Advance() override;
	virtual void Reset() override;

	virtual size_t GetCount() const override;
	virtual ff::IJoystickDevice* GetJoystick(size_t nJoy) const override;

private:
	ff::Vector<ff::ComPtr<ff::IJoystickDevice>, 4> _joysticks;
};

BEGIN_INTERFACES(JoystickInput)
	HAS_INTERFACE(IJoystickInput)
END_INTERFACES()

bool ff::CreateJoystickInput(ff::IJoystickInput** ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;

	ComPtr<JoystickInput> pInput;
	assertRetVal(SUCCEEDED(ff::ComAllocator<JoystickInput>::CreateInstance(&pInput)), false);
	assertRetVal(pInput->Init(), false);

	*ppInput = pInput.Detach();
	return true;
}

JoystickInput::JoystickInput()
{
}

JoystickInput::~JoystickInput()
{
}

bool JoystickInput::Init()
{
	for (DWORD i = 0; i < ff::GetMaxXboxJoystickCount(); i++)
	{
		XINPUT_CAPABILITIES caps;
		ff::ZeroObject(caps);
		DWORD result = ::XInputGetCapabilities(i, 0, &caps);

		if (result == ERROR_SUCCESS || result == ERROR_DEVICE_NOT_CONNECTED)
		{
			ff::ComPtr<ff::IJoystickDevice> pDevice;
			assertRetVal(ff::CreateXboxJoystick(i, &pDevice), false);
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

void JoystickInput::Reset()
{
	_joysticks.Clear();
	verify(Init());
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

#endif
