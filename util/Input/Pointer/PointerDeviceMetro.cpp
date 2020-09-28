#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/MetroGlobals.h"
#include "Input/DeviceEvent.h"
#include "Input/InputMapping.h"
#include "Input/Pointer/PointerDevice.h"
#include "Module/Module.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"

#if METRO_APP

static bool IsMouseEvent(Windows::UI::Input::PointerPoint^ point)
{
	return point->PointerDevice->PointerDeviceType == Windows::Devices::Input::PointerDeviceType::Mouse;
}

static bool IsMouseEvent(Windows::UI::Core::PointerEventArgs^ args)
{
	return ::IsMouseEvent(args->CurrentPoint);
}

class PointerDevice;

ref class PointerEvents
{
internal:
	PointerEvents();
	bool Init(ff::MetroGlobals* globals);

	void AddDevice(PointerDevice* device);
	void RemoveDevice(PointerDevice* device);
	ff::MetroGlobals* GetGlobals() const;

public:
	virtual ~PointerEvents();

private:
	void OnPointerEntered(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerExited(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerMoved(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerPressed(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerReleased(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerCaptureLost(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerWheelChanged(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);

	ff::MetroGlobals* _globals;
	ff::Vector<PointerDevice*, 8> _devices;
	ff::WinHandle _event;
	Windows::UI::Core::CoreIndependentInputSource^ _inputSource;
};

class __declspec(uuid("08dfcb6a-7d69-4e5a-aed3-628dd89a3e8d"))
	PointerDevice
	: public ff::ComBase
	, public ff::IPointerDevice
	, public ff::IDeviceEventProvider
{
public:
	DECLARE_HEADER(PointerDevice);

	bool Init(Platform::Object^ pointerEventsObject);
	void Destroy();

	// IInputDevice
	virtual void Advance() override;
	virtual void KillPending() override;
	virtual bool IsConnected() const override;

	// IPointerDevice
	virtual bool IsInWindow() const override;
	virtual ff::PointDouble GetPos() const override;
	virtual ff::PointDouble GetRelativePos() const override;

	virtual bool GetButton(int vkButton) const override;
	virtual int GetButtonClickCount(int vkButton) const override;
	virtual int GetButtonReleaseCount(int vkButton) const override;
	virtual int GetButtonDoubleClickCount(int vkButton) const override;
	virtual ff::PointDouble GetWheelScroll() const override;

	virtual size_t GetTouchCount() const override;
	virtual const ff::TouchInfo& GetTouchInfo(size_t index) const override;

	// IDeviceEventProvider
	virtual void SetSink(ff::IDeviceEventSink* sink) override;

	// Callbacks
	void OnPointerEntered(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerExited(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerMoved(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerPressed(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerReleased(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerCaptureLost(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);
	void OnPointerWheelChanged(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args);

private:
	ff::DeviceEvent OnMouseMoved(Windows::UI::Input::PointerPoint^ point);
	ff::DeviceEvent OnMousePressed(Windows::UI::Input::PointerPoint^ point);
	ff::DeviceEvent OnTouchMoved(Windows::UI::Input::PointerPoint^ point);
	ff::DeviceEvent OnTouchPressed(Windows::UI::Input::PointerPoint^ point);
	ff::DeviceEvent OnTouchReleased(Windows::UI::Input::PointerPoint^ point);

	struct MouseInfo
	{
		static const size_t BUTTON_COUNT = 7;

		ff::PointDouble _pos;
		ff::PointDouble _posRelative;
		ff::PointDouble _wheel;
		bool _buttons[BUTTON_COUNT];
		BYTE _clicks[BUTTON_COUNT];
		BYTE _releases[BUTTON_COUNT];
		BYTE _doubleClicks[BUTTON_COUNT];
	};

	struct InternalTouchInfo
	{
		InternalTouchInfo()
		{
			ff::ZeroObject(info);
		}

		Windows::UI::Input::PointerPoint^ point;
		ff::TouchInfo info;
	};

	InternalTouchInfo* FindTouchInfo(Windows::UI::Input::PointerPoint^ point, bool allowCreate);
	void UpdateTouchInfo(InternalTouchInfo& info, Windows::UI::Input::PointerPoint^ point);

	ff::Mutex _mutex;
	ff::ComPtr<ff::IDeviceEventSink> _sink;
	PointerEvents^ _events;
	MouseInfo _mouse;
	MouseInfo _pendingMouse;
	ff::Vector<InternalTouchInfo> _touches;
	ff::Vector<InternalTouchInfo> _pendingTouches;
	bool _insideWindow;
	bool _pendingInsideWindow;
};

BEGIN_INTERFACES(PointerDevice)
	HAS_INTERFACE(ff::IPointerDevice)
	HAS_INTERFACE(ff::IInputDevice)
	HAS_INTERFACE(ff::IDeviceEventProvider)
END_INTERFACES()

Platform::Object^ ff::CreatePointerEvents(MetroGlobals* globals)
{
	PointerEvents^ events = ref new PointerEvents();
	assertRetVal(events->Init(globals), nullptr);
	return events;
}

ff::ComPtr<ff::IPointerDevice> ff::CreatePointerDevice(Platform::Object^ pointerEventsObject)
{
	ComPtr<PointerDevice, IPointerDevice> myObj;
	assertHrRetVal(ComAllocator<PointerDevice>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(pointerEventsObject), false);

	return myObj.Detach();
}

PointerEvents::PointerEvents()
	: _globals(nullptr)
{
}

PointerEvents::~PointerEvents()
{
	if (_inputSource)
	{
		_inputSource->Dispatcher->StopProcessEvents();
		ff::WaitForHandle(_event);
	}

	_inputSource = nullptr;
	_devices.Clear();
}

bool PointerEvents::Init(ff::MetroGlobals* globals)
{
	assertRetVal(globals && !_globals, false);
	_globals = globals;
	_event = ff::CreateEvent();

	ff::GetThreadPool()->AddThread([this, globals]()
		{
			::SetThreadDescription(::GetCurrentThread(), L"ff : Pointer Input");

			_inputSource = globals->GetSwapChainPanel()->CreateCoreIndependentInputSource(
				Windows::UI::Core::CoreInputDeviceTypes::Mouse |
				Windows::UI::Core::CoreInputDeviceTypes::Touch |
				Windows::UI::Core::CoreInputDeviceTypes::Pen);

			Windows::Foundation::EventRegistrationToken tokens[7];
			tokens[0] = _inputSource->PointerEntered +=
				ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerEntered);
			tokens[1] = _inputSource->PointerExited +=
				ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerExited);
			tokens[2] = _inputSource->PointerMoved +=
				ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerMoved);
			tokens[3] = _inputSource->PointerPressed +=
				ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerPressed);
			tokens[4] = _inputSource->PointerReleased +=
				ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerReleased);
			tokens[5] = _inputSource->PointerCaptureLost +=
				ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerCaptureLost);
			tokens[6] = _inputSource->PointerWheelChanged +=
				ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerWheelChanged);

			::SetEvent(_event);

			_inputSource->Dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessUntilQuit);

			_inputSource->PointerEntered -= tokens[0];
			_inputSource->PointerExited -= tokens[1];
			_inputSource->PointerMoved -= tokens[2];
			_inputSource->PointerPressed -= tokens[3];
			_inputSource->PointerReleased -= tokens[4];
			_inputSource->PointerCaptureLost -= tokens[5];
			_inputSource->PointerWheelChanged -= tokens[6];

			::SetEvent(_event);
		});

	ff::WaitForEventAndReset(_event);

	return true;
}

void PointerEvents::AddDevice(PointerDevice* device)
{
	assertRet(_inputSource && device);

	Platform::WeakReference weakSelf(this);
	auto handler = ref new Windows::UI::Core::DispatchedHandler([weakSelf, device]()
		{
			PointerEvents^ self = weakSelf.Resolve<PointerEvents>();
			if (self && !self->_devices.Contains(device))
			{
				self->_devices.Push(device);
			}
		});

	_inputSource->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, handler);
}

void PointerEvents::RemoveDevice(PointerDevice* device)
{
	assertRet(_inputSource && device);

	Platform::WeakReference weakSelf(this);
	auto handler = ref new Windows::UI::Core::DispatchedHandler([weakSelf, device]()
		{
			PointerEvents^ self = weakSelf.Resolve<PointerEvents>();
			if (self)
			{
				verify(self->_devices.DeleteItem(device));
				::SetEvent(self->_event);
			}
		});

	_inputSource->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, handler);
	ff::WaitForEventAndReset(_event);
}

ff::MetroGlobals* PointerEvents::GetGlobals() const
{
	return _globals;
}

void PointerEvents::OnPointerEntered(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	for (PointerDevice* device : _devices)
	{
		device->OnPointerEntered(sender, args);
	}
}

void PointerEvents::OnPointerExited(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	for (PointerDevice* device : _devices)
	{
		device->OnPointerExited(sender, args);
	}
}

void PointerEvents::OnPointerMoved(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	for (PointerDevice* device : _devices)
	{
		device->OnPointerMoved(sender, args);
	}
}

void PointerEvents::OnPointerPressed(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	for (PointerDevice* device : _devices)
	{
		device->OnPointerPressed(sender, args);
	}
}

void PointerEvents::OnPointerReleased(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	for (PointerDevice* device : _devices)
	{
		device->OnPointerReleased(sender, args);
	}
}

void PointerEvents::OnPointerCaptureLost(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	for (PointerDevice* device : _devices)
	{
		device->OnPointerCaptureLost(sender, args);
	}
}

void PointerEvents::OnPointerWheelChanged(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	for (PointerDevice* device : _devices)
	{
		device->OnPointerWheelChanged(sender, args);
	}
}

PointerDevice::PointerDevice()
	: _insideWindow(false)
	, _pendingInsideWindow(false)
{
	ff::ZeroObject(_mouse);
	ff::ZeroObject(_pendingMouse);
}

PointerDevice::~PointerDevice()
{
	Destroy();
}

bool PointerDevice::Init(Platform::Object^ pointerEventsObject)
{
	PointerEvents^ events = dynamic_cast<PointerEvents^>(pointerEventsObject);
	assertRetVal(!_events && events, false);

	_events = events;
	_events->AddDevice(this);

	return true;
}

void PointerDevice::Destroy()
{
	if (_events != nullptr)
	{
		_events->RemoveDevice(this);
		_events = nullptr;
	}

	_touches.Clear();
	_pendingTouches.Clear();
}

void PointerDevice::Advance()
{
	ff::LockMutex lock(_mutex);

	for (InternalTouchInfo& info : _pendingTouches)
	{
		info.info.counter++;
	}

	_mouse = _pendingMouse;
	_touches = _pendingTouches;
	_insideWindow = _pendingInsideWindow;

	ff::ZeroObject(_pendingMouse._posRelative);
	ff::ZeroObject(_pendingMouse._wheel);
	ff::ZeroObject(_pendingMouse._clicks);
	ff::ZeroObject(_pendingMouse._releases);
	ff::ZeroObject(_pendingMouse._doubleClicks);
}

void PointerDevice::KillPending()
{
	ff::Vector<ff::DeviceEvent, 8> deviceEvents;
	{
		ff::LockMutex lock(_mutex);

		for (unsigned int i = 0; i < MouseInfo::BUTTON_COUNT; i++)
		{
			if (_pendingMouse._buttons[i])
			{
				_pendingMouse._buttons[i] = false;

				if (_pendingMouse._releases[i] != 0xFF)
				{
					_pendingMouse._releases[i]++;
				}

				deviceEvents.Push(ff::DeviceEventMousePress(i, 0, _pendingMouse._pos.ToType<int>()));
			}
		}

		ff::Vector<Windows::UI::Input::PointerPoint^, 16> points;
		points.Reserve(_pendingTouches.Size());

		for (InternalTouchInfo& info : _pendingTouches)
		{
			points.Push(info.point);
		}

		for (Windows::UI::Input::PointerPoint^ point : points)
		{
			ff::DeviceEvent deviceEvent = OnTouchReleased(point);
			if (!::IsMouseEvent(point))
			{
				deviceEvents.Push(deviceEvent);
			}
		}

		assert(_pendingTouches.IsEmpty());
	}

	if (_sink)
	{
		for (const ff::DeviceEvent& deviceEvent : deviceEvents)
		{
			_sink->AddEvent(deviceEvent);
		}
	}
}

bool PointerDevice::IsConnected() const
{
	return true;
}

bool PointerDevice::IsInWindow() const
{
	return _insideWindow;
}

ff::PointDouble PointerDevice::GetPos() const
{
	return _mouse._pos;
}

ff::PointDouble PointerDevice::GetRelativePos() const
{
	return _mouse._posRelative;
}

static bool IsValidButton(int vkButton)
{
	switch (vkButton)
	{
	case VK_LBUTTON:
	case VK_RBUTTON:
	case VK_MBUTTON:
	case VK_XBUTTON1:
	case VK_XBUTTON2:
		return true;

	default:
		assertRetVal(false, false);
	}
}

bool PointerDevice::GetButton(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._buttons[vkButton] : false;
}

int PointerDevice::GetButtonClickCount(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._clicks[vkButton] : 0;
}

int PointerDevice::GetButtonReleaseCount(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._releases[vkButton] : 0;
}

int PointerDevice::GetButtonDoubleClickCount(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._doubleClicks[vkButton] : 0;
}

ff::PointDouble PointerDevice::GetWheelScroll() const
{
	return _mouse._wheel;
}

size_t PointerDevice::GetTouchCount() const
{
	return _touches.Size();
}

const ff::TouchInfo& PointerDevice::GetTouchInfo(size_t index) const
{
	return _touches[index].info;
}

void PointerDevice::SetSink(ff::IDeviceEventSink* sink)
{
	_sink = sink;
}

void PointerDevice::OnPointerEntered(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	if (::IsMouseEvent(args))
	{
		ff::LockMutex lock(_mutex);
		_pendingInsideWindow = true;
	}
}

void PointerDevice::OnPointerExited(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	if (::IsMouseEvent(args))
	{
		ff::LockMutex lock(_mutex);
		_pendingInsideWindow = false;
	}
}

void PointerDevice::OnPointerMoved(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	ff::DeviceEvent deviceEvent;
	{
		ff::LockMutex lock(_mutex);

		deviceEvent = OnTouchMoved(args->CurrentPoint);

		if (::IsMouseEvent(args))
		{
			deviceEvent = OnMouseMoved(args->CurrentPoint);
		}
	}

	if (_sink)
	{
		_sink->AddEvent(deviceEvent);
	}
}

void PointerDevice::OnPointerPressed(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	ff::DeviceEvent deviceEvent;
	{
		ff::LockMutex lock(_mutex);

		deviceEvent = OnTouchPressed(args->CurrentPoint);

		if (::IsMouseEvent(args))
		{
			deviceEvent = OnMousePressed(args->CurrentPoint);
		}
	}

	if (_sink)
	{
		_sink->AddEvent(deviceEvent);
	}
}

void PointerDevice::OnPointerReleased(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	ff::DeviceEvent deviceEvent;
	{
		ff::LockMutex lock(_mutex);

		deviceEvent = OnTouchReleased(args->CurrentPoint);

		if (::IsMouseEvent(args))
		{
			deviceEvent = OnMousePressed(args->CurrentPoint);
		}
	}

	if (_sink)
	{
		_sink->AddEvent(deviceEvent);
	}
}

void PointerDevice::OnPointerCaptureLost(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	KillPending();
}

void PointerDevice::OnPointerWheelChanged(Platform::Object^ sender, Windows::UI::Core::PointerEventArgs^ args)
{
	Windows::UI::Input::PointerPoint^ point = args->CurrentPoint;
	int delta = point->Properties->MouseWheelDelta;
	double dpiScale = _events->GetGlobals()->GetDpiScale();
	ff::PointDouble pos(point->Position.X * dpiScale, point->Position.Y * dpiScale);

	ff::LockMutex lock(_mutex);

	if (args->CurrentPoint->Properties->IsHorizontalMouseWheel)
	{
		_pendingMouse._wheel.x += delta;

		if (_sink)
		{
			_sink->AddEvent(ff::DeviceEventMouseWheelX(delta, pos.ToType<int>()));
		}
	}
	else
	{
		_pendingMouse._wheel.y += delta;

		if (_sink)
		{
			_sink->AddEvent(ff::DeviceEventMouseWheelY(delta, pos.ToType<int>()));
		}
	}
}

ff::DeviceEvent PointerDevice::OnMouseMoved(Windows::UI::Input::PointerPoint^ point)
{
	double dpiScale = _events->GetGlobals()->GetDpiScale();

	_pendingMouse._pos.x = point->Position.X * dpiScale;
	_pendingMouse._pos.y = point->Position.Y * dpiScale;
	_pendingMouse._posRelative = _pendingMouse._pos - _mouse._pos;

	return ff::DeviceEventMouseMove(_pendingMouse._pos.ToType<int>());
}

ff::DeviceEvent PointerDevice::OnMousePressed(Windows::UI::Input::PointerPoint^ point)
{
	int nPresses = 0;
	int vkButton = 0;

	switch (point->Properties->PointerUpdateKind)
	{
	case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
		vkButton = VK_LBUTTON;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
		vkButton = VK_LBUTTON;
		break;

	case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
		vkButton = VK_RBUTTON;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
		vkButton = VK_RBUTTON;
		break;

	case Windows::UI::Input::PointerUpdateKind::MiddleButtonPressed:
		vkButton = VK_MBUTTON;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::MiddleButtonReleased:
		vkButton = VK_MBUTTON;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton1Pressed:
		vkButton = VK_XBUTTON1;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton1Released:
		vkButton = VK_XBUTTON1;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton2Pressed:
		vkButton = VK_XBUTTON2;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton2Released:
		vkButton = VK_XBUTTON2;
		break;
	}

	if (vkButton)
	{
		switch (nPresses)
		{
		case 2:
			if (_pendingMouse._doubleClicks[vkButton] != 0xFF)
			{
				_pendingMouse._doubleClicks[vkButton]++;
			}
			__fallthrough;

		case 1:
			_pendingMouse._buttons[vkButton] = true;

			if (_pendingMouse._clicks[vkButton] != 0xFF)
			{
				_pendingMouse._clicks[vkButton]++;
			}
			break;

		case 0:
			_pendingMouse._buttons[vkButton] = false;

			if (_pendingMouse._releases[vkButton] != 0xFF)
			{
				_pendingMouse._releases[vkButton]++;
			}
			break;
		}

		OnMouseMoved(point);

		return ff::DeviceEventMousePress(vkButton, nPresses, _pendingMouse._pos.ToType<int>());
	}

	return ff::DeviceEvent();
}

ff::DeviceEvent PointerDevice::OnTouchMoved(Windows::UI::Input::PointerPoint^ point)
{
	InternalTouchInfo* info = FindTouchInfo(point, false);
	if (info)
	{
		return ff::DeviceEventTouchMove(info->info.id, info->info.pos.ToType<int>());
	}

	return ff::DeviceEvent();
}

ff::DeviceEvent PointerDevice::OnTouchPressed(Windows::UI::Input::PointerPoint^ point)
{
	InternalTouchInfo* info = FindTouchInfo(point, true);
	if (info)
	{
		return ff::DeviceEventTouchPress(info->info.id, 1, info->info.pos.ToType<int>());
	}

	return ff::DeviceEvent();
}

ff::DeviceEvent PointerDevice::OnTouchReleased(Windows::UI::Input::PointerPoint^ point)
{
	InternalTouchInfo* info = FindTouchInfo(point, false);
	if (info)
	{
		_pendingTouches.Delete(info - _pendingTouches.Data());
		return ff::DeviceEventTouchPress(info->info.id, 0, info->info.pos.ToType<int>());
	}

	return ff::DeviceEvent();
}

PointerDevice::InternalTouchInfo* PointerDevice::FindTouchInfo(Windows::UI::Input::PointerPoint^ point, bool allowCreate)
{
	for (InternalTouchInfo& info : _pendingTouches)
	{
		if (point->PointerId == info.point->PointerId)
		{
			UpdateTouchInfo(info, point);
			return &info;
		}
	}

	if (allowCreate)
	{
		InternalTouchInfo newInfo;
		UpdateTouchInfo(newInfo, point);
		newInfo.info.startPos = newInfo.info.pos;
		_pendingTouches.Push(newInfo);
		return &_pendingTouches.GetLast();
	}

	return nullptr;
}

void PointerDevice::UpdateTouchInfo(InternalTouchInfo& info, Windows::UI::Input::PointerPoint^ point)
{
	ff::InputDevice type = ff::INPUT_DEVICE_NULL;
	unsigned int id = point->PointerId;
	unsigned int vk = 0;
	double dpiScale = _events->GetGlobals()->GetDpiScale();
	ff::PointDouble pos(point->Position.X * dpiScale, point->Position.Y * dpiScale);

	switch (point->PointerDevice->PointerDeviceType)
	{
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		type = ff::INPUT_DEVICE_MOUSE;
		break;

	case Windows::Devices::Input::PointerDeviceType::Pen:
		type = ff::INPUT_DEVICE_PEN;
		break;

	case Windows::Devices::Input::PointerDeviceType::Touch:
		type = ff::INPUT_DEVICE_TOUCH;
		break;
	}

	if (point->Properties->IsLeftButtonPressed)
	{
		vk = VK_LBUTTON;
	}
	else if (point->Properties->IsRightButtonPressed)
	{
		vk = VK_RBUTTON;
	}
	else if (point->Properties->IsMiddleButtonPressed)
	{
		vk = VK_MBUTTON;
	}
	else if (point->Properties->IsXButton1Pressed)
	{
		vk = VK_XBUTTON1;
	}
	else if (point->Properties->IsXButton2Pressed)
	{
		vk = VK_XBUTTON2;
	}

	info.point = point;
	info.info.type = type;
	info.info.id = id;
	info.info.vk = vk;
	info.info.pos = pos;
}

#endif // METRO_APP
