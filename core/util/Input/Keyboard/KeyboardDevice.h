#pragma once

#include "Input/InputDevice.h"

namespace ff
{
	class __declspec(uuid("85404e39-cfbe-4551-9afa-bcc726ae7b02")) __declspec(novtable)
		IKeyboardDevice : public IInputDevice
	{
	public:
		virtual bool GetKey(int vk) const = 0;
		virtual int GetKeyPressCount(int vk) const = 0; // since last Advance()
		virtual String GetChars() const = 0;
	};

#if METRO_APP
	bool CreateKeyboardDevice(Windows::UI::Xaml::Window^ window, IKeyboardDevice** obj);
#else
	bool CreateKeyboardDevice(HWND hwnd, IKeyboardDevice** obj);
#endif
}
