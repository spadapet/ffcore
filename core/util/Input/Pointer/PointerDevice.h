#pragma once

#include "Input/InputDevice.h"

namespace ff
{
	enum InputDevice : unsigned char;

	struct TouchInfo
	{
		InputDevice type;
		PointDouble startPos;
		PointDouble pos;
		unsigned int id;
		unsigned int counter;
		unsigned int vk;
	};

	class __declspec(uuid("dc258f87-ba84-4b7a-b75a-3ce57ad23dc9")) __declspec(novtable)
		IPointerDevice : public IInputDevice
	{
	public:
		virtual bool IsInWindow() const = 0;
		virtual PointDouble GetPos() const = 0; // in window coordinates
		virtual PointDouble GetRelativePos() const = 0; // since last Advance()

		virtual bool GetButton(int vkButton) const = 0;
		virtual int GetButtonClickCount(int vkButton) const = 0; // since the last Advance()
		virtual int GetButtonReleaseCount(int vkButton) const = 0; // since the last Advance()
		virtual int GetButtonDoubleClickCount(int vkButton) const = 0; // since the last Advance()
		virtual PointDouble GetWheelScroll() const = 0; // since the last Advance()

		virtual size_t GetTouchCount() const = 0;
		virtual const TouchInfo& GetTouchInfo(size_t index) const = 0;
	};

#if METRO_APP
	class MetroGlobals;

	Platform::Object^ CreatePointerEvents(MetroGlobals* globals);
	ff::ComPtr<IPointerDevice> CreatePointerDevice(Platform::Object^ pointerEventsObject);
#else
	bool CreatePointerDevice(HWND hwnd, ff::IPointerDevice** ppInput);
#endif
}
