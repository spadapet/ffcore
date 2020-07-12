#include "pch.h"
#include "UI/Internal/XamlKeyMap.h"

struct XamlKeyMap
{
	XamlKeyMap()
	{
		std::memset(_keys.data(), 0, _keys.size() * sizeof(Noesis::Key));

		_keys[VK_BACK] = Noesis::Key_Back;
		_keys[VK_TAB] = Noesis::Key_Tab;
		_keys[VK_CLEAR] = Noesis::Key_Clear;
		_keys[VK_RETURN] = Noesis::Key_Return;
		_keys[VK_PAUSE] = Noesis::Key_Pause;

		_keys[VK_SHIFT] = Noesis::Key_LeftShift;
		_keys[VK_LSHIFT] = Noesis::Key_LeftShift;
		_keys[VK_RSHIFT] = Noesis::Key_RightShift;
		_keys[VK_CONTROL] = Noesis::Key_LeftCtrl;
		_keys[VK_LCONTROL] = Noesis::Key_LeftCtrl;
		_keys[VK_RCONTROL] = Noesis::Key_RightCtrl;
		_keys[VK_MENU] = Noesis::Key_LeftAlt;
		_keys[VK_LMENU] = Noesis::Key_LeftAlt;
		_keys[VK_RMENU] = Noesis::Key_RightAlt;
		_keys[VK_LWIN] = Noesis::Key_LWin;
		_keys[VK_RWIN] = Noesis::Key_RWin;
		_keys[VK_ESCAPE] = Noesis::Key_Escape;

		_keys[VK_SPACE] = Noesis::Key_Space;
		_keys[VK_PRIOR] = Noesis::Key_Prior;
		_keys[VK_NEXT] = Noesis::Key_Next;
		_keys[VK_END] = Noesis::Key_End;
		_keys[VK_HOME] = Noesis::Key_Home;
		_keys[VK_LEFT] = Noesis::Key_Left;
		_keys[VK_UP] = Noesis::Key_Up;
		_keys[VK_RIGHT] = Noesis::Key_Right;
		_keys[VK_DOWN] = Noesis::Key_Down;
		_keys[VK_SELECT] = Noesis::Key_Select;
		_keys[VK_PRINT] = Noesis::Key_Print;
		_keys[VK_EXECUTE] = Noesis::Key_Execute;
		_keys[VK_SNAPSHOT] = Noesis::Key_Snapshot;
		_keys[VK_INSERT] = Noesis::Key_Insert;
		_keys[VK_DELETE] = Noesis::Key_Delete;
		_keys[VK_HELP] = Noesis::Key_Help;

		_keys['0'] = Noesis::Key_D0;
		_keys['1'] = Noesis::Key_D1;
		_keys['2'] = Noesis::Key_D2;
		_keys['3'] = Noesis::Key_D3;
		_keys['4'] = Noesis::Key_D4;
		_keys['5'] = Noesis::Key_D5;
		_keys['6'] = Noesis::Key_D6;
		_keys['7'] = Noesis::Key_D7;
		_keys['8'] = Noesis::Key_D8;
		_keys['9'] = Noesis::Key_D9;

		_keys[VK_NUMPAD0] = Noesis::Key_NumPad0;
		_keys[VK_NUMPAD1] = Noesis::Key_NumPad1;
		_keys[VK_NUMPAD2] = Noesis::Key_NumPad2;
		_keys[VK_NUMPAD3] = Noesis::Key_NumPad3;
		_keys[VK_NUMPAD4] = Noesis::Key_NumPad4;
		_keys[VK_NUMPAD5] = Noesis::Key_NumPad5;
		_keys[VK_NUMPAD6] = Noesis::Key_NumPad6;
		_keys[VK_NUMPAD7] = Noesis::Key_NumPad7;
		_keys[VK_NUMPAD8] = Noesis::Key_NumPad8;
		_keys[VK_NUMPAD9] = Noesis::Key_NumPad9;

		_keys[VK_MULTIPLY] = Noesis::Key_Multiply;
		_keys[VK_ADD] = Noesis::Key_Add;
		_keys[VK_SEPARATOR] = Noesis::Key_Separator;
		_keys[VK_SUBTRACT] = Noesis::Key_Subtract;
		_keys[VK_DECIMAL] = Noesis::Key_Decimal;
		_keys[VK_DIVIDE] = Noesis::Key_Divide;

		_keys['A'] = Noesis::Key_A;
		_keys['B'] = Noesis::Key_B;
		_keys['C'] = Noesis::Key_C;
		_keys['D'] = Noesis::Key_D;
		_keys['E'] = Noesis::Key_E;
		_keys['F'] = Noesis::Key_F;
		_keys['G'] = Noesis::Key_G;
		_keys['H'] = Noesis::Key_H;
		_keys['I'] = Noesis::Key_I;
		_keys['J'] = Noesis::Key_J;
		_keys['K'] = Noesis::Key_K;
		_keys['L'] = Noesis::Key_L;
		_keys['M'] = Noesis::Key_M;
		_keys['N'] = Noesis::Key_N;
		_keys['O'] = Noesis::Key_O;
		_keys['P'] = Noesis::Key_P;
		_keys['Q'] = Noesis::Key_Q;
		_keys['R'] = Noesis::Key_R;
		_keys['S'] = Noesis::Key_S;
		_keys['T'] = Noesis::Key_T;
		_keys['U'] = Noesis::Key_U;
		_keys['V'] = Noesis::Key_V;
		_keys['W'] = Noesis::Key_W;
		_keys['X'] = Noesis::Key_X;
		_keys['Y'] = Noesis::Key_Y;
		_keys['Z'] = Noesis::Key_Z;

		_keys[VK_F1] = Noesis::Key_F1;
		_keys[VK_F2] = Noesis::Key_F2;
		_keys[VK_F3] = Noesis::Key_F3;
		_keys[VK_F4] = Noesis::Key_F4;
		_keys[VK_F5] = Noesis::Key_F5;
		_keys[VK_F6] = Noesis::Key_F6;
		_keys[VK_F7] = Noesis::Key_F7;
		_keys[VK_F8] = Noesis::Key_F8;
		_keys[VK_F9] = Noesis::Key_F9;
		_keys[VK_F10] = Noesis::Key_F10;
		_keys[VK_F11] = Noesis::Key_F11;
		_keys[VK_F12] = Noesis::Key_F12;
		_keys[VK_F13] = Noesis::Key_F13;
		_keys[VK_F14] = Noesis::Key_F14;
		_keys[VK_F15] = Noesis::Key_F15;
		_keys[VK_F16] = Noesis::Key_F16;
		_keys[VK_F17] = Noesis::Key_F17;
		_keys[VK_F18] = Noesis::Key_F18;
		_keys[VK_F19] = Noesis::Key_F19;
		_keys[VK_F20] = Noesis::Key_F20;
		_keys[VK_F21] = Noesis::Key_F21;
		_keys[VK_F22] = Noesis::Key_F22;
		_keys[VK_F23] = Noesis::Key_F23;
		_keys[VK_F24] = Noesis::Key_F24;

		_keys[VK_NUMLOCK] = Noesis::Key_NumLock;
		_keys[VK_SCROLL] = Noesis::Key_Scroll;

		_keys[VK_GAMEPAD_DPAD_LEFT] = Noesis::Key_GamepadLeft;
		_keys[VK_GAMEPAD_DPAD_UP] = Noesis::Key_GamepadUp;
		_keys[VK_GAMEPAD_DPAD_RIGHT] = Noesis::Key_GamepadRight;
		_keys[VK_GAMEPAD_DPAD_DOWN] = Noesis::Key_GamepadDown;
		_keys[VK_GAMEPAD_LEFT_THUMBSTICK_LEFT] = Noesis::Key_GamepadLeft;
		_keys[VK_GAMEPAD_LEFT_THUMBSTICK_UP] = Noesis::Key_GamepadUp;
		_keys[VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT] = Noesis::Key_GamepadRight;
		_keys[VK_GAMEPAD_LEFT_THUMBSTICK_DOWN] = Noesis::Key_GamepadDown;
		_keys[VK_GAMEPAD_A] = Noesis::Key_GamepadAccept;
		_keys[VK_GAMEPAD_B] = Noesis::Key_GamepadCancel;
		_keys[VK_GAMEPAD_MENU] = Noesis::Key_GamepadMenu;
		_keys[VK_GAMEPAD_VIEW] = Noesis::Key_GamepadView;
	}

	Noesis::Key GetKey(unsigned int vk)
	{
		return (vk < _keys.size()) ? _keys[vk] : Noesis::Key::Key_None;
	}

	bool IsValid(unsigned int vk)
	{
		return GetKey(vk) != Noesis::Key_None;
	}

	std::array<Noesis::Key, 256> _keys;

} s_keyMap;

bool ff::IsValidKey(unsigned int vk)
{
	return s_keyMap.IsValid(vk);
}

Noesis::Key ff::GetKey(unsigned int vk)
{
	return s_keyMap.GetKey(vk);
}

bool ff::IsValidMouseButton(unsigned int vk)
{
	switch (vk)
	{
	case VK_LBUTTON:
	case VK_RBUTTON:
	case VK_MBUTTON:
	case VK_XBUTTON1:
	case VK_XBUTTON2:
		return true;

	default:
		return false;
	}
}

Noesis::MouseButton ff::GetMouseButton(unsigned int vk)
{
	switch (vk)
	{
	case VK_LBUTTON:
		return Noesis::MouseButton_Left;
	case VK_RBUTTON:
		return Noesis::MouseButton_Right;
	case VK_MBUTTON:
		return Noesis::MouseButton_Middle;
	case VK_XBUTTON1:
		return Noesis::MouseButton_XButton1;
	case VK_XBUTTON2:
		return Noesis::MouseButton_XButton2;
	default:
		assertRetVal(false, Noesis::MouseButton_Count);
	}
}
