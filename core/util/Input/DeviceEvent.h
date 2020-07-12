#pragma once

namespace ff
{
	enum class DeviceEventType
	{
		None,
		KeyPress,
		KeyChar,
		MousePress,
		MouseMove,
		MouseWheelX,
		MouseWheelY,
		TouchPress,
		TouchMove,
	};

	struct DeviceEvent
	{
		DeviceEvent();
		DeviceEvent(DeviceEventType type, unsigned int id = 0, int count = 0, PointInt pos = PointInt::Zeros());

		DeviceEventType _type;
		unsigned int _id;
		int _count;
		PointInt _pos;
	};

	struct DeviceEventKeyPress : public DeviceEvent
	{
		DeviceEventKeyPress(unsigned int vk, int count);
	};

	struct DeviceEventKeyChar : public DeviceEvent
	{
		DeviceEventKeyChar(unsigned int ch);
	};

	struct DeviceEventMousePress : public DeviceEvent
	{
		DeviceEventMousePress(unsigned int vk, int count, PointInt pos);
	};

	struct DeviceEventMouseMove : public DeviceEvent
	{
		DeviceEventMouseMove(PointInt pos);
	};

	struct DeviceEventMouseWheelX : public DeviceEvent
	{
		DeviceEventMouseWheelX(int scroll, PointInt pos);
	};

	struct DeviceEventMouseWheelY : public DeviceEvent
	{
		DeviceEventMouseWheelY(int scroll, PointInt pos);
	};

	struct DeviceEventTouchPress : public DeviceEvent
	{
		DeviceEventTouchPress(unsigned int id, int count, PointInt pos);
	};

	struct DeviceEventTouchMove : public DeviceEvent
	{
		DeviceEventTouchMove(unsigned int id, PointInt pos);
	};

	class IDeviceEvents
	{
	public:
		virtual const Vector<DeviceEvent>& GetEvents() const = 0;
	};

	class __declspec(uuid("9752e98e-fc94-44bf-98bf-9c69566aa37a")) __declspec(novtable)
		IDeviceEventSink : public IUnknown, public IDeviceEvents
	{
	public:
		virtual bool Advance() = 0;
		virtual void AddEvent(const DeviceEvent& event) = 0;
	};

	class __declspec(uuid("05f343fe-e1aa-4e30-b49c-998c93eb4f3d")) __declspec(novtable)
		IDeviceEventProvider : public IUnknown
	{
	public:
		virtual void SetSink(IDeviceEventSink* sink) = 0;
	};

	UTIL_API ComPtr<IDeviceEventSink> CreateDeviceEventSink();
}

MAKE_POD(ff::DeviceEvent);
MAKE_POD(ff::DeviceEventKeyPress);
MAKE_POD(ff::DeviceEventKeyChar);
MAKE_POD(ff::DeviceEventMousePress);
MAKE_POD(ff::DeviceEventMouseMove);
MAKE_POD(ff::DeviceEventMouseWheelX);
MAKE_POD(ff::DeviceEventMouseWheelY);
MAKE_POD(ff::DeviceEventTouchPress);
MAKE_POD(ff::DeviceEventTouchMove);
