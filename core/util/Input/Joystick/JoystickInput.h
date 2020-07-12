#pragma once

#include "Input/InputDevice.h"

namespace ff
{
	class IJoystickDevice;

	class __declspec(uuid("1ba6b747-4375-41cb-a0a2-340287461bb1")) __declspec(novtable)
		IJoystickInput : public IInputDevice
	{
	public:
		virtual size_t GetCount() const = 0;
		virtual IJoystickDevice* GetJoystick(size_t nJoy) const = 0;
	};

#if METRO_APP
	bool CreateJoystickInput(IJoystickInput** obj);
#else
	bool CreateJoystickInput(HWND hwnd, IJoystickInput** obj);
#endif
}
