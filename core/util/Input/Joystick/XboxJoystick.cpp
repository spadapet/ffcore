#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ThreadGlobals.h"
#include "Input/DeviceEvent.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Module/Module.h"
#include "Windows/WinUtil.h"

#if !METRO_APP

size_t ff::GetMaxXboxJoystickCount()
{
	return 4;
}

class __declspec(uuid("cdf625a6-b259-428d-a515-f40207899393"))
	XboxJoystick
	: public ff::ComBase
	, public ff::IJoystickDevice
	, public ff::IDeviceEventProvider
	, public ff::IWindowProcListener
{
public:
	DECLARE_HEADER(XboxJoystick);

	bool Init(HWND hwnd, size_t index);
	void Destroy();

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

	// IDeviceEventProvider
	virtual void SetSink(ff::IDeviceEventSink* sink) override;

	// IWindowProcListener
	virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult) override;

private:
	static const int VK_GAMEPAD_FIRST = VK_GAMEPAD_A;
	static const int VK_GAMEPAD_COUNT = VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT - VK_GAMEPAD_A + 1;

	void ClearState(bool buttonPresses);
	void UpdateButtonPresses();
	void UpdateButtonPressCount(int vk, bool pressed);
	static int ChooseRandomConnectionCount();

	HWND _hwnd;
	XINPUT_STATE _state;
	DWORD _index;
	int _checkConnected;
	bool _connected;
	float _triggerPressing[2];
	ff::PointFloat _stickPressing[2];
	std::array<int, VK_GAMEPAD_COUNT> _buttonPressCount;
	ff::ComPtr<ff::IDeviceEventSink> _sink;
};

BEGIN_INTERFACES(XboxJoystick)
	HAS_INTERFACE(ff::IJoystickDevice)
	HAS_INTERFACE(ff::IInputDevice)
	HAS_INTERFACE(ff::IDeviceEventProvider)
END_INTERFACES()

bool ff::CreateXboxJoystick(HWND hwnd, size_t index, ff::IJoystickDevice** ppDevice)
{
	assertRetVal(ppDevice, false);
	*ppDevice = nullptr;

	ff::ComPtr<XboxJoystick, ff::IJoystickDevice> pDevice;
	assertRetVal(SUCCEEDED(ff::ComAllocator<XboxJoystick>::CreateInstance(&pDevice)), false);
	assertRetVal(pDevice->Init(hwnd, index), false);

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
	, _hwnd(nullptr)
{
	ClearState(true);
}

XboxJoystick::~XboxJoystick()
{
	Destroy();
}

bool XboxJoystick::Init(HWND hwnd, size_t index)
{
	assertRetVal(hwnd && index >= 0 && index < ff::GetMaxXboxJoystickCount(), false);

	_hwnd = hwnd;
	_index = (DWORD)index;
	_connected = (::XInputGetState(_index, &_state) == ERROR_SUCCESS);
	_checkConnected = XboxJoystick::ChooseRandomConnectionCount();

	ff::ThreadGlobals::Get()->AddWindowListener(_hwnd, this);

	return true;
}

void XboxJoystick::Destroy()
{
	if (_hwnd)
	{
		ff::ThreadGlobals::Get()->RemoveWindowListener(_hwnd, this);
		_hwnd = nullptr;
	}
}

void XboxJoystick::Advance()
{
	DWORD hr = (_connected || _checkConnected == 0) ? ::XInputGetState(_index, &_state) : ERROR_DEVICE_NOT_CONNECTED;

	if (hr == ERROR_SUCCESS)
	{
		_connected = true;
	}
	else
	{
		ClearState(false);

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

	UpdateButtonPresses();
}

void XboxJoystick::KillPending()
{
	ClearState(false);
	UpdateButtonPresses();
}

void XboxJoystick::SetSink(ff::IDeviceEventSink* sink)
{
	_sink = sink;
}

bool XboxJoystick::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult)
{
	switch (msg)
	{
	case WM_DESTROY:
		Destroy();
		break;
	}

	return false;
}

void XboxJoystick::ClearState(bool buttonPresses)
{
	ff::ZeroObject(_state);
	ff::ZeroObject(_triggerPressing);
	ff::ZeroObject(_stickPressing);

	if (buttonPresses)
	{
		ff::ZeroObject(_buttonPressCount);
	}
}

void XboxJoystick::UpdateButtonPresses()
{
	UpdateButtonPressCount(VK_GAMEPAD_A, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_B, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_X, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_Y, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_RIGHT_SHOULDER, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_LEFT_SHOULDER, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_DPAD_UP, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_DPAD_DOWN, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_DPAD_LEFT, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_DPAD_RIGHT, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_MENU, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_VIEW, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0);
	UpdateButtonPressCount(VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON, (_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0);

	float t0 = _state.Gamepad.bLeftTrigger / 255.0f;
	float t1 = _state.Gamepad.bRightTrigger / 255.0f;

	float x0 = AnalogShortToFloat(_state.Gamepad.sThumbLX);
	float y0 = -AnalogShortToFloat(_state.Gamepad.sThumbLY);
	float x1 = AnalogShortToFloat(_state.Gamepad.sThumbRX);
	float y1 = -AnalogShortToFloat(_state.Gamepad.sThumbRY);

	const float rmax = 0.55f;
	const float rmin = 0.50f;

	// Left trigger press
	if (t0 >= rmax) _triggerPressing[0] = 1;
	else if (t0 <= rmin) _triggerPressing[0] = 0;
	UpdateButtonPressCount(VK_GAMEPAD_LEFT_TRIGGER, _triggerPressing[0] == 1);

	// Right trigger press
	if (t1 >= rmax) _triggerPressing[1] = 1;
	else if (t1 <= rmin) _triggerPressing[1] = 0;
	UpdateButtonPressCount(VK_GAMEPAD_RIGHT_TRIGGER, _triggerPressing[1] == 1);

	// Left stick X press
	if (x0 >= rmax) _stickPressing[0].x = 1;
	else if (x0 <= -rmax) _stickPressing[0].x = -1;
	else if (std::fabsf(x0) <= rmin) _stickPressing[0].x = 0;
	UpdateButtonPressCount(VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT, _stickPressing[0].x == 1);
	UpdateButtonPressCount(VK_GAMEPAD_LEFT_THUMBSTICK_LEFT, _stickPressing[0].x == -1);

	// Left stick Y press
	if (y0 >= rmax) _stickPressing[0].y = 1;
	else if (y0 <= -rmax) _stickPressing[0].y = -1;
	else if (std::fabsf(y0) <= rmin) _stickPressing[0].y = 0;
	UpdateButtonPressCount(VK_GAMEPAD_LEFT_THUMBSTICK_DOWN, _stickPressing[0].y == 1);
	UpdateButtonPressCount(VK_GAMEPAD_LEFT_THUMBSTICK_UP, _stickPressing[0].y == -1);

	// Right stick X press
	if (x1 >= rmax) _stickPressing[1].x = 1;
	else if (x1 <= -rmax) _stickPressing[1].x = -1;
	else if (std::fabsf(x1) <= rmin) _stickPressing[1].x = 0;
	UpdateButtonPressCount(VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT, _stickPressing[1].x == 1);
	UpdateButtonPressCount(VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT, _stickPressing[1].x == -1);

	// Right stick Y press
	if (y1 >= rmax) _stickPressing[1].y = 1;
	else if (y1 <= -rmax) _stickPressing[1].y = -1;
	else if (std::fabsf(y1) <= rmin) _stickPressing[1].y = 0;
	UpdateButtonPressCount(VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN, _stickPressing[1].y == 1);
	UpdateButtonPressCount(VK_GAMEPAD_RIGHT_THUMBSTICK_UP, _stickPressing[1].y == -1);
}

void XboxJoystick::UpdateButtonPressCount(int vk, bool pressed)
{
	const int buttonIndex = vk - VK_GAMEPAD_FIRST;
	if (pressed)
	{
		size_t count = (size_t)++_buttonPressCount[buttonIndex];

		if (_sink)
		{
			const size_t firstRepeat = 30;
			const size_t repeatCount = 6;

			if (count == 1)
			{
				_sink->AddEvent(ff::DeviceEventKeyPress(vk, 1));
			}
			else if (count > firstRepeat && (count - firstRepeat) % repeatCount == 0)
			{
				size_t repeats = (count - firstRepeat) / repeatCount + 1;
				_sink->AddEvent(ff::DeviceEventKeyPress(vk, (int)repeats));
			}
		}
	}
	else if (_buttonPressCount[buttonIndex])
	{
		_buttonPressCount[buttonIndex] = 0;

		if (_sink)
		{
			_sink->AddEvent(ff::DeviceEventKeyPress(vk, 0));
		}
	}
}

int XboxJoystick::ChooseRandomConnectionCount()
{
	return ff::ADVANCES_PER_SECOND_I + std::rand() % ff::ADVANCES_PER_SECOND_I;
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
		press.left = _buttonPressCount[VK_GAMEPAD_LEFT_THUMBSTICK_LEFT - VK_GAMEPAD_FIRST];
		press.right = _buttonPressCount[VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT - VK_GAMEPAD_FIRST];
		press.top = _buttonPressCount[VK_GAMEPAD_LEFT_THUMBSTICK_UP - VK_GAMEPAD_FIRST];
		press.bottom = _buttonPressCount[VK_GAMEPAD_LEFT_THUMBSTICK_DOWN - VK_GAMEPAD_FIRST];
		break;

	case 1:
		press.left = _buttonPressCount[VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT - VK_GAMEPAD_FIRST];
		press.right = _buttonPressCount[VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT - VK_GAMEPAD_FIRST];
		press.top = _buttonPressCount[VK_GAMEPAD_RIGHT_THUMBSTICK_UP - VK_GAMEPAD_FIRST];
		press.bottom = _buttonPressCount[VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN - VK_GAMEPAD_FIRST];
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
		press.left = _buttonPressCount[VK_GAMEPAD_DPAD_LEFT - VK_GAMEPAD_FIRST];
		press.right = _buttonPressCount[VK_GAMEPAD_DPAD_RIGHT - VK_GAMEPAD_FIRST];
		press.top = _buttonPressCount[VK_GAMEPAD_DPAD_UP - VK_GAMEPAD_FIRST];
		press.bottom = _buttonPressCount[VK_GAMEPAD_DPAD_DOWN - VK_GAMEPAD_FIRST];
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
	case 0: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
	case 1: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
	case 2: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
	case 3: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
	case 4: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
	case 5: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
	case 6: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
	case 7: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
	case 8: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
	case 9: return (_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
	default: assertRetVal(false, false);
	}
}

int XboxJoystick::GetButtonPressCount(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return _buttonPressCount[VK_GAMEPAD_A - VK_GAMEPAD_FIRST];
	case 1: return _buttonPressCount[VK_GAMEPAD_B - VK_GAMEPAD_FIRST];
	case 2: return _buttonPressCount[VK_GAMEPAD_X - VK_GAMEPAD_FIRST];
	case 3: return _buttonPressCount[VK_GAMEPAD_Y - VK_GAMEPAD_FIRST];
	case 4: return _buttonPressCount[VK_GAMEPAD_VIEW - VK_GAMEPAD_FIRST];
	case 5: return _buttonPressCount[VK_GAMEPAD_MENU - VK_GAMEPAD_FIRST];
	case 6: return _buttonPressCount[VK_GAMEPAD_LEFT_SHOULDER - VK_GAMEPAD_FIRST];
	case 7: return _buttonPressCount[VK_GAMEPAD_RIGHT_SHOULDER - VK_GAMEPAD_FIRST];
	case 8: return _buttonPressCount[VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON - VK_GAMEPAD_FIRST];
	case 9: return _buttonPressCount[VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON - VK_GAMEPAD_FIRST];
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
	case 0: return _buttonPressCount[VK_GAMEPAD_LEFT_TRIGGER - VK_GAMEPAD_FIRST];
	case 1: return _buttonPressCount[VK_GAMEPAD_RIGHT_TRIGGER - VK_GAMEPAD_FIRST];
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
