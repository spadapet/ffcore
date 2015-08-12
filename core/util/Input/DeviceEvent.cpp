#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/DeviceEvent.h"

ff::DeviceEvent::DeviceEvent()
	: DeviceEvent(DeviceEventType::None)
{
}

ff::DeviceEvent::DeviceEvent(DeviceEventType type, unsigned int id, int count, PointInt pos)
	: _type(type)
	, _id(id)
	, _count(count)
	, _pos(pos)
{
}

ff::DeviceEventKeyPress::DeviceEventKeyPress(unsigned int vk, int count)
	: DeviceEvent(ff::DeviceEventType::KeyPress, vk, count)
{
}

ff::DeviceEventKeyChar::DeviceEventKeyChar(unsigned int ch)
	: DeviceEvent(ff::DeviceEventType::KeyChar, ch)
{
}

ff::DeviceEventMousePress::DeviceEventMousePress(unsigned int vk, int count, PointInt pos)
	: DeviceEvent(ff::DeviceEventType::MousePress, vk, count, pos)
{
}

ff::DeviceEventMouseMove::DeviceEventMouseMove(PointInt pos)
	: DeviceEvent(ff::DeviceEventType::MouseMove, 0, 0, pos)
{
}

ff::DeviceEventMouseWheelX::DeviceEventMouseWheelX(int scroll, PointInt pos)
	: DeviceEvent(ff::DeviceEventType::MouseWheelX, 0, scroll, pos)
{
}

ff::DeviceEventMouseWheelY::DeviceEventMouseWheelY(int scroll, PointInt pos)
	: DeviceEvent(ff::DeviceEventType::MouseWheelY, 0, scroll, pos)
{
}

ff::DeviceEventTouchPress::DeviceEventTouchPress(unsigned int id, int count, PointInt pos)
	: DeviceEvent(ff::DeviceEventType::TouchPress, id, count, pos)
{
}

ff::DeviceEventTouchMove::DeviceEventTouchMove(unsigned int id, PointInt pos)
	: DeviceEvent(ff::DeviceEventType::TouchMove, id, 0, pos)
{
}

class __declspec(uuid("f3e82038-0197-4daa-ad59-5bd02b11fee9"))
	DeviceEventSink
	: public ff::ComBase
	, public ff::IDeviceEventSink
{
public:
	DECLARE_HEADER(DeviceEventSink);

	// IDeviceEvents
	virtual const ff::Vector<ff::DeviceEvent>& GetEvents() const override;

	// IDeviceEventSink
	virtual bool Advance() override;
	virtual void KillPending() override;
	virtual void Pause() override;
	virtual void Unpause() override;
	virtual void AddEvent(const ff::DeviceEvent& event) override;

private:
	ff::Mutex _mutex;
	ff::Vector<ff::DeviceEvent> _events;
	ff::Vector<ff::DeviceEvent> _eventsPending;
	bool _paused;
};

BEGIN_INTERFACES(DeviceEventSink)
	HAS_INTERFACE(ff::IDeviceEventSink)
END_INTERFACES()

ff::ComPtr<ff::IDeviceEventSink> ff::CreateDeviceEventSink()
{
	ff::ComPtr<DeviceEventSink, IDeviceEventSink> obj;
	assertHrRetVal(ff::ComAllocator<DeviceEventSink>::CreateInstance(&obj), nullptr);
	return obj.Interface();
}

DeviceEventSink::DeviceEventSink()
	: _paused(false)
{
}

DeviceEventSink::~DeviceEventSink()
{
}

const ff::Vector<ff::DeviceEvent>& DeviceEventSink::GetEvents() const
{
	return _events;
}

bool DeviceEventSink::Advance()
{
	ff::LockMutex lock(_mutex);
	_events = _eventsPending;
	_eventsPending.Clear();

	return !_events.IsEmpty();
}

void DeviceEventSink::KillPending()
{
	ff::LockMutex lock(_mutex);
	_eventsPending.Clear();
}

void DeviceEventSink::Pause()
{
	ff::LockMutex lock(_mutex);
	_paused = true;
}

void DeviceEventSink::Unpause()
{
	ff::LockMutex lock(_mutex);
	_paused = false;
}

void DeviceEventSink::AddEvent(const ff::DeviceEvent& event)
{
	if (event._type != ff::DeviceEventType::None)
	{
		ff::LockMutex lock(_mutex);
		if (!_paused)
		{
			_eventsPending.Push(event);
		}
	}
}
