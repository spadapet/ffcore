#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Module/Module.h"

#if !METRO_APP

size_t ff::GetMaxXboxJoystickCount()
{
	return 4;
}

class __declspec(uuid("cdf625a6-b259-428d-a515-f40207899393"))
	XboxJoystick : public ff::ComBase, public ff::IJoystickDevice
{
public:
	DECLARE_HEADER(XboxJoystick);

	bool Init(size_t index);

	// IInputDevice
	virtual void Advance() override;
	virtual void KillPending() override;
	virtual bool IsConnected() const override;

	// IJoystickDevice
	virtual size_t GetStickCount() const override;
	virtual ff::PointFloat GetStickPos(size_t nStick, bool bDigital) const override;
	virtual ff::RectInt GetStickPressCount(size_t nStick) const override;
	virtual ff::String GetStickName(size_t nStick) const override;

	virtual size_t GetDPadCount() const override;
	virtual ff::PointInt GetDPadPos(size_t nDPad) const override;
	virtual ff::RectInt GetDPadPressCount(size_t nDPad) const override;
	virtual ff::String GetDPadName(size_t nDPad) const override;

	virtual size_t GetButtonCount() const override;
	virtual bool GetButton(size_t nButton) const override;
	virtual int GetButtonPressCount(size_t nButton) const override;
	virtual ff::String GetButtonName(size_t nButton) const override;

	virtual size_t GetTriggerCount() const override;
	virtual float GetTrigger(size_t nTrigger, bool bDigital) const override;
	virtual int GetTriggerPressCount(size_t nTrigger) const override;
	virtual ff::String GetTriggerName(size_t nTrigger) const override;

	virtual bool HasKeyButton(int vk) const override;
	virtual bool GetKeyButton(int vk) const override;
	virtual int GetKeyButtonPressCount(int vk) const override;
	virtual ff::String GetKeyButtonName(int vk) const override;

private:
	void CheckPresses(WORD prevButtons);
	static int ChooseRandomConnectionCount();

	BYTE& GetPressed(int vk);
	BYTE GetPressed(int vk) const;
	bool IsButtonPressed(WORD buttonFlag) const;

	static const int VK_GAMEPAD_FIRST = VK_GAMEPAD_A;
	static const int VK_GAMEPAD_COUNT = VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT - VK_GAMEPAD_A + 1;

	XINPUT_STATE _state;
	DWORD _index;
	int _checkConnected;
	bool _connected;
	float _triggerPressing[2];
	ff::PointFloat _stickPressing[2];
	BYTE _pressed[VK_GAMEPAD_COUNT];
};

BEGIN_INTERFACES(XboxJoystick)
	HAS_INTERFACE(IJoystickDevice)
	HAS_INTERFACE(IInputDevice)
END_INTERFACES()

bool ff::CreateXboxJoystick(size_t index, ff::IJoystickDevice** ppDevice)
{
	assertRetVal(ppDevice, false);
	*ppDevice = nullptr;

	ff::ComPtr<XboxJoystick> pDevice;
	assertRetVal(SUCCEEDED(ff::ComAllocator<XboxJoystick>::CreateInstance(&pDevice)), false);
	assertRetVal(pDevice->Init(index), false);

	*ppDevice = pDevice.Detach();
	return true;
}

static float AnalogShortToFloat(SHORT sh)
{
	return sh / ((sh < 0) ? 32768.0f : 32767.0f);
}

XboxJoystick::XboxJoystick()
	: _index(0)
	, _checkConnected(0)
	, _connected(true)
{
	static_assert(_countof(_pressed) == 24);

	ff::ZeroObject(_state);
	ff::ZeroObject(_triggerPressing);
	ff::ZeroObject(_stickPressing);
	ff::ZeroObject(_pressed);
}

XboxJoystick::~XboxJoystick()
{
}

bool XboxJoystick::Init(size_t index)
{
	assertRetVal(index >= 0 && index < ff::GetMaxXboxJoystickCount(), false);

	_index = (DWORD)index;
	_connected = (::XInputGetState(_index, &_state) == ERROR_SUCCESS);
	_checkConnected = XboxJoystick::ChooseRandomConnectionCount();

	return true;
}


void XboxJoystick::Advance()
{
	ff::ZeroObject(_pressed);

	XINPUT_STATE prevState = _state;
	DWORD hr = (_connected || _checkConnected == 0)
		? XInputGetState(_index, &_state)
		: ERROR_DEVICE_NOT_CONNECTED;

	if (hr == ERROR_SUCCESS)
	{
		_connected = true;
		CheckPresses(prevState.Gamepad.wButtons);
	}
	else
	{
		ff::ZeroObject(_state);
		ff::ZeroObject(_triggerPressing);
		ff::ZeroObject(_stickPressing);

		if (_connected || _checkConnected == 0)
		{
			_connected = false;
			_checkConnected = XboxJoystick::ChooseRandomConnectionCount();
		}
		else
		{
			_checkConnected--;
		}
	}
}

void XboxJoystick::KillPending()
{
	// state is polled, so there is never pending input
}

void XboxJoystick::CheckPresses(WORD prevButtons)
{
	WORD cb = _state.Gamepad.wButtons;

	if (cb)
	{
		GetPressed(VK_GAMEPAD_A) = (cb & XINPUT_GAMEPAD_A) && !(prevButtons & XINPUT_GAMEPAD_A);
		GetPressed(VK_GAMEPAD_B) = (cb & XINPUT_GAMEPAD_B) && !(prevButtons & XINPUT_GAMEPAD_B);
		GetPressed(VK_GAMEPAD_X) = (cb & XINPUT_GAMEPAD_X) && !(prevButtons & XINPUT_GAMEPAD_X);
		GetPressed(VK_GAMEPAD_Y) = (cb & XINPUT_GAMEPAD_Y) && !(prevButtons & XINPUT_GAMEPAD_Y);
		GetPressed(VK_GAMEPAD_RIGHT_SHOULDER) = (cb & XINPUT_GAMEPAD_RIGHT_SHOULDER) && !(prevButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
		GetPressed(VK_GAMEPAD_LEFT_SHOULDER) = (cb & XINPUT_GAMEPAD_LEFT_SHOULDER) && !(prevButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
		GetPressed(VK_GAMEPAD_DPAD_UP) = (cb & XINPUT_GAMEPAD_DPAD_UP) && !(prevButtons & XINPUT_GAMEPAD_DPAD_UP);
		GetPressed(VK_GAMEPAD_DPAD_DOWN) = (cb & XINPUT_GAMEPAD_DPAD_DOWN) && !(prevButtons & XINPUT_GAMEPAD_DPAD_DOWN);
		GetPressed(VK_GAMEPAD_DPAD_LEFT) = (cb & XINPUT_GAMEPAD_DPAD_LEFT) && !(prevButtons & XINPUT_GAMEPAD_DPAD_LEFT);
		GetPressed(VK_GAMEPAD_DPAD_RIGHT) = (cb & XINPUT_GAMEPAD_DPAD_RIGHT) && !(prevButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
		GetPressed(VK_GAMEPAD_MENU) = (cb & XINPUT_GAMEPAD_START) && !(prevButtons & XINPUT_GAMEPAD_START);
		GetPressed(VK_GAMEPAD_VIEW) = (cb & XINPUT_GAMEPAD_BACK) && !(prevButtons & XINPUT_GAMEPAD_BACK);
		GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON) = (cb & XINPUT_GAMEPAD_LEFT_THUMB) && !(prevButtons & XINPUT_GAMEPAD_LEFT_THUMB);
		GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON) = (cb & XINPUT_GAMEPAD_RIGHT_THUMB) && !(prevButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
	}

	float t0 = _state.Gamepad.bLeftTrigger / 255.0f;
	float t1 = _state.Gamepad.bRightTrigger / 255.0f;

	float x0 = AnalogShortToFloat(_state.Gamepad.sThumbLX);
	float y0 = -AnalogShortToFloat(_state.Gamepad.sThumbLY);
	float x1 = AnalogShortToFloat(_state.Gamepad.sThumbRX);
	float y1 = -AnalogShortToFloat(_state.Gamepad.sThumbRY);

	const float rmax = 0.55f;
	const float rmin = 0.50f;

	// Left trigger press
	if (t0 >= rmax) { GetPressed(VK_GAMEPAD_LEFT_TRIGGER) = (_triggerPressing[0] == 0); _triggerPressing[0] = 1; }
	else if (t0 <= rmin) { _triggerPressing[0] = 0; }

	// Right trigger press
	if (t1 >= rmax) { GetPressed(VK_GAMEPAD_RIGHT_TRIGGER) = (_triggerPressing[1] == 0); _triggerPressing[1] = 1; }
	else if (t1 <= rmin) { _triggerPressing[1] = 0; }

	// Left stick X press
	if (x0 >= rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT) = (_stickPressing[0].x != 1); _stickPressing[0].x = 1; }
	else if (x0 <= -rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_LEFT) = (_stickPressing[0].x != -1); _stickPressing[0].x = -1; }
	else if (std::fabsf(x0) <= rmin) { _stickPressing[0].x = 0; }

	// Left stick Y press
	if (y0 >= rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_DOWN) = (_stickPressing[0].y != 1); _stickPressing[0].y = 1; }
	else if (y0 <= -rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_UP) = (_stickPressing[0].y != -1); _stickPressing[0].y = -1; }
	else if (std::fabsf(y0) <= rmin) { _stickPressing[0].y = 0; }

	// Right stick X press
	if (x1 >= rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT) = (_stickPressing[1].x != 1); _stickPressing[1].x = 1; }
	else if (x1 <= -rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT) = (_stickPressing[1].x != -1); _stickPressing[1].x = -1; }
	else if (std::fabsf(x1) <= rmin) { _stickPressing[1].x = 0; }

	// Right stick Y press
	if (y1 >= rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN) = (_stickPressing[1].y != 1); _stickPressing[1].y = 1; }
	else if (y1 <= -rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_UP) = (_stickPressing[1].y != -1); _stickPressing[1].y = -1; }
	else if (std::fabsf(y1) <= rmin) { _stickPressing[1].y = 0; }
}

int XboxJoystick::ChooseRandomConnectionCount()
{
	return 60 + std::rand() % 60;
}

bool XboxJoystick::IsConnected() const
{
	return _connected;
}

size_t XboxJoystick::GetStickCount() const
{
	return 2;
}

ff::PointFloat XboxJoystick::GetStickPos(size_t nStick, bool bDigital) const
{
	ff::PointFloat pos(0, 0);

	if (bDigital)
	{
		switch (nStick)
		{
		case 0: return _stickPressing[0]; break;
		case 1: return _stickPressing[1]; break;
		default: assert(false);
		}
	}
	else
	{
		switch (nStick)
		{
		case 0:
			pos.x = AnalogShortToFloat(_state.Gamepad.sThumbLX);
			pos.y = -AnalogShortToFloat(_state.Gamepad.sThumbLY);
			break;

		case 1:
			pos.x = AnalogShortToFloat(_state.Gamepad.sThumbRX);
			pos.y = -AnalogShortToFloat(_state.Gamepad.sThumbRY);
			break;

		default:
			assert(false);
		}
	}

	return pos;
}

ff::RectInt XboxJoystick::GetStickPressCount(size_t nStick) const
{
	ff::RectInt press(0, 0, 0, 0);

	switch (nStick)
	{
	case 0:
		press.left = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_LEFT);
		press.right = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT);
		press.top = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_UP);
		press.bottom = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_DOWN);
		break;

	case 1:
		press.left = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT);
		press.right = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT);
		press.top = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_UP);
		press.bottom = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN);
		break;

	default:
		assert(false);
	}

	return press;
}

ff::String XboxJoystick::GetStickName(size_t nStick) const
{
	switch (nStick)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_PAD_LEFT"));
	case 1: return ff::GetThisModule().GetString(ff::String(L"XBOX_PAD_RIGHT"));
	default: assertRetVal(false, ff::GetThisModule().GetString(ff::String(L"INPUT_STICK_UNKNOWN")));
	}
}

size_t XboxJoystick::GetDPadCount() const
{
	return 1;
}

ff::PointInt XboxJoystick::GetDPadPos(size_t nDPad) const
{
	ff::PointInt pos(0, 0);

	if (!nDPad)
	{
		pos.x = (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? -1 : ((_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1 : 0);
		pos.y = (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) ? -1 : ((_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? 1 : 0);
	}

	return pos;
}

ff::RectInt XboxJoystick::GetDPadPressCount(size_t nDPad) const
{
	ff::RectInt press(0, 0, 0, 0);

	if (!nDPad)
	{
		press.left = GetPressed(VK_GAMEPAD_DPAD_LEFT);
		press.right = GetPressed(VK_GAMEPAD_DPAD_RIGHT);
		press.top = GetPressed(VK_GAMEPAD_DPAD_UP);
		press.bottom = GetPressed(VK_GAMEPAD_DPAD_DOWN);
	}

	return press;
}

ff::String XboxJoystick::GetDPadName(size_t nDPad) const
{
	switch (nDPad)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_PAD_DIGITAL"));
	default: assertRetVal(false, ff::GetThisModule().GetString(ff::String(L"INPUT_STICK_UNKNOWN")));
	}
}

size_t XboxJoystick::GetButtonCount() const
{
	return 10;
}

bool XboxJoystick::GetButton(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return IsButtonPressed(XINPUT_GAMEPAD_A);
	case 1: return IsButtonPressed(XINPUT_GAMEPAD_B);
	case 2: return IsButtonPressed(XINPUT_GAMEPAD_X);
	case 3: return IsButtonPressed(XINPUT_GAMEPAD_Y);
	case 4: return IsButtonPressed(XINPUT_GAMEPAD_BACK);
	case 5: return IsButtonPressed(XINPUT_GAMEPAD_START);
	case 6: return IsButtonPressed(XINPUT_GAMEPAD_LEFT_SHOULDER);
	case 7: return IsButtonPressed(XINPUT_GAMEPAD_RIGHT_SHOULDER);
	case 8: return IsButtonPressed(XINPUT_GAMEPAD_LEFT_THUMB);
	case 9: return IsButtonPressed(XINPUT_GAMEPAD_RIGHT_THUMB);
	default: assertRetVal(false, false);
	}
}

int XboxJoystick::GetButtonPressCount(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return GetPressed(VK_GAMEPAD_A);
	case 1: return GetPressed(VK_GAMEPAD_B);
	case 2: return GetPressed(VK_GAMEPAD_X);
	case 3: return GetPressed(VK_GAMEPAD_Y);
	case 4: return GetPressed(VK_GAMEPAD_VIEW);
	case 5: return GetPressed(VK_GAMEPAD_MENU);
	case 6: return GetPressed(VK_GAMEPAD_LEFT_SHOULDER);
	case 7: return GetPressed(VK_GAMEPAD_RIGHT_SHOULDER);
	case 8: return GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON);
	case 9: return GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON);
	default: assertRetVal(false, 0);
	}
}

ff::String XboxJoystick::GetButtonName(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_A"));
	case 1: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_B"));
	case 2: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_X"));
	case 3: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_Y"));
	case 4: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_BACK"));
	case 5: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_START"));
	case 6: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_LSHOULDER"));
	case 7: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_RSHOULDER"));
	case 8: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_LTHUMB"));
	case 9: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_RTHUMB"));
	default: assertRetVal(false, ff::String());
	}
}

size_t XboxJoystick::GetTriggerCount() const
{
	return 2;
}

float XboxJoystick::GetTrigger(size_t nTrigger, bool bDigital) const
{
	if (bDigital)
	{
		switch (nTrigger)
		{
		case 0: return _triggerPressing[0];
		case 1: return _triggerPressing[1];
		default: assertRetVal(false, 0);
		}
	}
	else
	{
		switch (nTrigger)
		{
		case 0: return _state.Gamepad.bLeftTrigger / 255.0f;
		case 1: return _state.Gamepad.bRightTrigger / 255.0f;
		default: assertRetVal(false, 0);
		}
	}
}

int XboxJoystick::GetTriggerPressCount(size_t nTrigger) const
{
	switch (nTrigger)
	{
	case 0: return GetPressed(VK_GAMEPAD_LEFT_TRIGGER);
	case 1: return GetPressed(VK_GAMEPAD_RIGHT_TRIGGER);
	default: assertRetVal(false, 0);
	}
}

ff::String XboxJoystick::GetTriggerName(size_t nTrigger) const
{
	switch (nTrigger)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_LTRIGGER"));
	case 1: return ff::GetThisModule().GetString(ff::String(L"XBOX_RTRIGGER"));
	default: assertRetVal(false, ff::String());
	}
}

BYTE& XboxJoystick::GetPressed(int vk)
{
	return _pressed[vk - VK_GAMEPAD_FIRST];
}

BYTE XboxJoystick::GetPressed(int vk) const
{
	return _pressed[vk - VK_GAMEPAD_FIRST];
}

bool XboxJoystick::IsButtonPressed(WORD buttonFlag) const
{
	return (_state.Gamepad.wButtons & buttonFlag) == buttonFlag;
}

bool XboxJoystick::HasKeyButton(int vk) const
{
	switch (vk)
	{
	case VK_GAMEPAD_VIEW:
	case VK_GAMEPAD_MENU:
	case VK_GAMEPAD_LEFT_SHOULDER:
	case VK_GAMEPAD_RIGHT_SHOULDER:
	case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON:
	case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON:
	case VK_GAMEPAD_A:
	case VK_GAMEPAD_B:
	case VK_GAMEPAD_X:
	case VK_GAMEPAD_Y:
		return true;
	}

	return false;
}

bool XboxJoystick::GetKeyButton(int vk) const
{
	switch (vk)
	{
	case VK_GAMEPAD_VIEW:
		return GetButton(4);

	case VK_GAMEPAD_MENU:
		return GetButton(5);

	case VK_GAMEPAD_LEFT_SHOULDER:
		return GetButton(6);

	case VK_GAMEPAD_RIGHT_SHOULDER:
		return GetButton(7);

	case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON:
		return GetButton(9);

	case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON:
		return GetButton(10);

	case VK_GAMEPAD_A:
		return GetButton(0);

	case VK_GAMEPAD_B:
		return GetButton(1);

	case VK_GAMEPAD_X:
		return GetButton(2);

	case VK_GAMEPAD_Y:
		return GetButton(3);
	}

	assertRetVal(false, false);
}

int XboxJoystick::GetKeyButtonPressCount(int vk) const
{
	switch (vk)
	{
	case VK_GAMEPAD_VIEW:
		return GetButtonPressCount(4);

	case VK_GAMEPAD_MENU:
		return GetButtonPressCount(5);

	case VK_GAMEPAD_LEFT_SHOULDER:
		return GetButtonPressCount(6);

	case VK_GAMEPAD_RIGHT_SHOULDER:
		return GetButtonPressCount(7);

	case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON:
		return GetButtonPressCount(9);

	case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON:
		return GetButtonPressCount(10);

	case VK_GAMEPAD_A:
		return GetButtonPressCount(0);

	case VK_GAMEPAD_B:
		return GetButtonPressCount(1);

	case VK_GAMEPAD_X:
		return GetButtonPressCount(2);

	case VK_GAMEPAD_Y:
		return GetButtonPressCount(3);
	}

	assertRetVal(false, 0);
}

ff::String XboxJoystick::GetKeyButtonName(int vk) const
{
	switch (vk)
	{
	case VK_GAMEPAD_VIEW:
		return GetButtonName(4);

	case VK_GAMEPAD_MENU:
		return GetButtonName(5);

	case VK_GAMEPAD_LEFT_SHOULDER:
		return GetButtonName(6);

	case VK_GAMEPAD_RIGHT_SHOULDER:
		return GetButtonName(7);

	case VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON:
		return GetButtonName(9);

	case VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON:
		return GetButtonName(10);

	case VK_GAMEPAD_A:
		return GetButtonName(0);

	case VK_GAMEPAD_B:
		return GetButtonName(1);

	case VK_GAMEPAD_X:
		return GetButtonName(2);

	case VK_GAMEPAD_Y:
		return GetButtonName(3);
	}

	assertRetVal(false, ff::String());
}

#endif
