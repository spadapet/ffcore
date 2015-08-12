#pragma once

#include "Input/InputDevice.h"

namespace ff
{
	class __declspec(uuid("f3ff5369-b610-4139-89cf-beda1cc4add0")) __declspec(novtable)
		IJoystickDevice : public IInputDevice
	{
	public:
		virtual size_t GetStickCount() const = 0;
		virtual PointFloat GetStickPos(size_t nStick, bool bDigital) const = 0;
		virtual RectInt GetStickPressCount(size_t nStick) const = 0;
		virtual String GetStickName(size_t nStick) const = 0;

		virtual size_t GetDPadCount() const = 0;
		virtual PointInt GetDPadPos(size_t nDPad) const = 0;
		virtual RectInt GetDPadPressCount(size_t nDPad) const = 0;
		virtual String GetDPadName(size_t nDPad) const = 0;

		virtual size_t GetButtonCount() const = 0;
		virtual bool GetButton(size_t nButton) const = 0;
		virtual int GetButtonPressCount(size_t nButton) const = 0;
		virtual String GetButtonName(size_t nButton) const = 0;

		virtual size_t GetTriggerCount() const = 0;
		virtual float GetTrigger(size_t nTrigger, bool bDigital) const = 0;
		virtual int GetTriggerPressCount(size_t nTrigger) const = 0;
		virtual String GetTriggerName(size_t nTrigger) const = 0;

		virtual bool HasKeyButton(int vk) const = 0;
		virtual bool GetKeyButton(int vk) const = 0;
		virtual int GetKeyButtonPressCount(int vk) const = 0;
		virtual String GetKeyButtonName(int vk) const = 0;
	};

#if METRO_APP

	class __declspec(uuid("55bf7390-265e-48a2-a915-b4c03d0d86a9")) __declspec(novtable)
		IXboxJoystick : public IJoystickDevice
	{
	public:
		virtual Windows::Gaming::Input::Gamepad^ GetGamepad() const = 0;
		virtual void SetGamepad(Windows::Gaming::Input::Gamepad^ gamepad) = 0;
	};

	bool CreateXboxJoystick(Windows::Gaming::Input::Gamepad^ gamepad, IXboxJoystick** device);

#else
	size_t GetMaxXboxJoystickCount();
	bool CreateXboxJoystick(size_t index, IJoystickDevice** device);
#endif
}
