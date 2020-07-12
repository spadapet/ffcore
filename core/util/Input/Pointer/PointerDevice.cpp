#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/AppGlobals.h"
#include "Globals/ThreadGlobals.h"
#include "Input/DeviceEvent.h"
#include "Input/InputMapping.h"
#include "Input/Pointer/PointerDevice.h"
#include "Module/Module.h"
#include "Windows/WinUtil.h"

#if !METRO_APP

static bool s_releasingCapture = false;

class __declspec(uuid("a2f6e2e1-d9e6-41e0-bd16-49396abcdc26"))
	PointerDevice
	: public ff::ComBase
	, public ff::IPointerDevice
	, public ff::IDeviceEventProvider
	, public ff::IWindowProcListener
{
public:
	DECLARE_HEADER(PointerDevice);

	bool Init(HWND hwnd);
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

	// IWindowProcListener
	virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult) override;

private:
	void OnMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void OnPointerMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

		ff::TouchInfo info;
	};

	InternalTouchInfo* FindTouchInfo(WPARAM wp, LPARAM lp, bool allowCreate);
	void UpdateTouchInfo(InternalTouchInfo& info, WPARAM wp, LPARAM lp);

	ff::Mutex _mutex;
	ff::ComPtr<ff::IDeviceEventSink> _sink;
	HWND _hwnd;
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

bool ff::CreatePointerDevice(HWND hwnd, ff::IPointerDevice** ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;

	ff::ComPtr<PointerDevice, IPointerDevice> pInput;
	assertHrRetVal(ff::ComAllocator<PointerDevice>::CreateInstance(&pInput), false);
	assertRetVal(pInput->Init(hwnd), false);

	*ppInput = pInput.Detach();
	return true;
}

PointerDevice::PointerDevice()
	: _hwnd(nullptr)
	, _insideWindow(false)
	, _pendingInsideWindow(false)
{
	ff::ZeroObject(_mouse);
	ff::ZeroObject(_pendingMouse);
}

PointerDevice::~PointerDevice()
{
	Destroy();
}

bool PointerDevice::Init(HWND hwnd)
{
	assertRetVal(hwnd, false);

	_hwnd = hwnd;
	ff::ThreadGlobals::Get()->AddWindowListener(_hwnd, this);

	return true;
}

void PointerDevice::Destroy()
{
	if (_hwnd)
	{
		ff::ThreadGlobals::Get()->RemoveWindowListener(_hwnd, this);
		_hwnd = nullptr;
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

		for (const InternalTouchInfo& info : _pendingTouches)
		{
			if (info.info.type != ff::INPUT_DEVICE_MOUSE)
			{
				deviceEvents.Push(ff::DeviceEventTouchPress(info.info.id, 0, info.info.pos.ToType<int>()));
			}
		}

		_pendingTouches.Clear();
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
	return IsValidButton(vkButton) ? _mouse._buttons[vkButton] : false;
}

int PointerDevice::GetButtonClickCount(int vkButton) const
{
	return IsValidButton(vkButton) ? _mouse._clicks[vkButton] : 0;
}

int PointerDevice::GetButtonReleaseCount(int vkButton) const
{
	return IsValidButton(vkButton) ? _mouse._releases[vkButton] : 0;
}

int PointerDevice::GetButtonDoubleClickCount(int vkButton) const
{
	return IsValidButton(vkButton) ? _mouse._doubleClicks[vkButton] : 0;
}

ff::PointDouble PointerDevice::GetWheelScroll() const
{
	return _mouse._wheel;
}

bool PointerDevice::ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult)
{
	switch (msg)
	{
	case WM_DESTROY:
		Destroy();
		break;

	case WM_MOUSELEAVE:
		OnMouseMessage(hwnd, msg, wParam, lParam);
		break;

	case WM_POINTERDOWN:
	case WM_POINTERUP:
	case WM_POINTERUPDATE:
		OnPointerMessage(hwnd, msg, wParam, lParam);
		break;

	case WM_POINTERCAPTURECHANGED:
		if (!s_releasingCapture)
		{
			OnPointerMessage(hwnd, msg, wParam, lParam);
		}
		break;

	case WM_CAPTURECHANGED:
		if (!s_releasingCapture)
		{
			KillPending();
		}
		break;

	default:
		if (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST)
		{
			OnMouseMessage(hwnd, msg, wParam, lParam);
		}
		break;
	}

	return false;
}

void PointerDevice::OnMouseMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	bool notifyMouseLeave = false;
	unsigned int nPresses = 0;
	unsigned int vkButton = 0;
	ff::PointInt mousePos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	ff::DeviceEvent deviceEvent(ff::DeviceEventType::None);

	ff::LockMutex lock(_mutex);

 	switch (msg)
	{
	case WM_LBUTTONDOWN:
		vkButton = VK_LBUTTON;
		nPresses = 1;
		break;

	case WM_LBUTTONUP:
		vkButton = VK_LBUTTON;
		break;

	case WM_LBUTTONDBLCLK:
		vkButton = VK_LBUTTON;
		nPresses = 2;
		break;

	case WM_RBUTTONDOWN:
		vkButton = VK_RBUTTON;
		nPresses = 1;
		break;

	case WM_RBUTTONUP:
		vkButton = VK_RBUTTON;
		break;

	case WM_RBUTTONDBLCLK:
		vkButton = VK_RBUTTON;
		nPresses = 2;
		break;

	case WM_MBUTTONDOWN:
		vkButton = VK_MBUTTON;
		nPresses = 1;
		break;

	case WM_MBUTTONUP:
		vkButton = VK_MBUTTON;
		break;

	case WM_MBUTTONDBLCLK:
		vkButton = VK_MBUTTON;
		nPresses = 2;
		break;

	case WM_XBUTTONDOWN:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case 1:
			vkButton = VK_XBUTTON1;
			nPresses = 1;
			break;

		case 2:
			vkButton = VK_XBUTTON2;
			nPresses = 1;
			break;
		}
		break;

	case WM_XBUTTONUP:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case 1:
			vkButton = VK_XBUTTON1;
			break;

		case 2:
			vkButton = VK_XBUTTON2;
			break;
		}
		break;

	case WM_XBUTTONDBLCLK:
		switch (GET_XBUTTON_WPARAM(wParam))
		{
		case 1:
			vkButton = VK_XBUTTON1;
			nPresses = 2;
			break;

		case 2:
			vkButton = VK_XBUTTON2;
			nPresses = 2;
			break;
		}
		break;

	case WM_MOUSEWHEEL:
		_pendingMouse._wheel.y += GET_WHEEL_DELTA_WPARAM(wParam);
		deviceEvent = ff::DeviceEventMouseWheelY(GET_WHEEL_DELTA_WPARAM(wParam), mousePos);
		break;

	case WM_MOUSEHWHEEL:
		_pendingMouse._wheel.x += GET_WHEEL_DELTA_WPARAM(wParam);
		deviceEvent = ff::DeviceEventMouseWheelX(GET_WHEEL_DELTA_WPARAM(wParam), mousePos);
		break;

	case WM_MOUSEMOVE:
		deviceEvent = ff::DeviceEventMouseMove(mousePos);

		if (!_pendingInsideWindow)
		{
			notifyMouseLeave = true;
			_pendingInsideWindow = true;
		}
		break;

	case WM_MOUSELEAVE:
		_pendingInsideWindow = false;
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

		deviceEvent = ff::DeviceEventMousePress(vkButton, nPresses, mousePos);
	}

	if (vkButton || msg == WM_MOUSEMOVE)
	{
		_pendingMouse._pos = mousePos.ToType<double>();
		_pendingMouse._posRelative = _pendingMouse._pos - _mouse._pos;
	}

	bool allButtonsUp = true;
	for (size_t i = 0; i < MouseInfo::BUTTON_COUNT; i++)
	{
		if (_pendingMouse._buttons[i])
		{
			allButtonsUp = false;
			break;
		}
	}

	lock.Unlock();

	if (_sink)
	{
		_sink->AddEvent(deviceEvent);
	}

	if (notifyMouseLeave)
	{
		TRACKMOUSEEVENT tme;
		ff::ZeroObject(tme);
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hwnd;
		::TrackMouseEvent(&tme);
	}

	if (vkButton)
	{
		if (nPresses)
		{
			if (::GetCapture() != hwnd)
			{
				::SetCapture(hwnd);
			}
		}
		else if (allButtonsUp && ::GetCapture() == hwnd)
		{
			s_releasingCapture = true;

			::ReleaseCapture();

			s_releasingCapture = false;
		}
	}
}

void PointerDevice::OnPointerMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ff::DeviceEvent deviceEvent(ff::DeviceEventType::None);
	InternalTouchInfo* info = nullptr;

	ff::LockMutex lock(_mutex);

	switch (msg)
	{
	case WM_POINTERDOWN:
		if ((info = FindTouchInfo(wParam, lParam, true)) != nullptr)
		{
			deviceEvent = ff::DeviceEventTouchPress(info->info.id, 1, info->info.pos.ToType<int>());
		}
		break;

	case WM_POINTERUPDATE:
		if ((info = FindTouchInfo(wParam, lParam, false)) != nullptr)
		{
			deviceEvent = ff::DeviceEventTouchMove(info->info.id, info->info.pos.ToType<int>());
		}
		break;

	case WM_POINTERUP:
	case WM_POINTERCAPTURECHANGED:
		if ((info = FindTouchInfo(wParam, lParam, false)) != nullptr)
		{
			_pendingTouches.Delete(info - _pendingTouches.Data());
			deviceEvent = ff::DeviceEventTouchPress(info->info.id, 0, info->info.pos.ToType<int>());
		}
		break;
	}

	lock.Unlock();

	if (_sink && info && info->info.type != ff::INPUT_DEVICE_MOUSE)
	{
		_sink->AddEvent(deviceEvent);
	}
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

PointerDevice::InternalTouchInfo* PointerDevice::FindTouchInfo(WPARAM wp, LPARAM lp, bool allowCreate)
{
	unsigned int id = GET_POINTERID_WPARAM(wp);

	for (InternalTouchInfo& info : _pendingTouches)
	{
		if (id == info.info.id)
		{
			UpdateTouchInfo(info, wp, lp);
			return &info;
		}
	}

	if (allowCreate)
	{
		InternalTouchInfo newInfo;
		UpdateTouchInfo(newInfo, wp, lp);
		newInfo.info.startPos = newInfo.info.pos;
		_pendingTouches.Push(newInfo);
		return &_pendingTouches.GetLast();
	}

	return nullptr;
}

void PointerDevice::UpdateTouchInfo(InternalTouchInfo& info, WPARAM wp, LPARAM lp)
{
	ff::InputDevice type = ff::INPUT_DEVICE_NULL;
	unsigned int id = GET_POINTERID_WPARAM(wp);
	unsigned int vk = 0;

	ff::PointInt pos(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
	assertRet(::ScreenToClient(_hwnd, (LPPOINT)&pos));

	POINTER_INFO idInfo;
	assertRet(::GetPointerInfo(id, &idInfo));

	switch (idInfo.pointerType)
	{
	case PT_POINTER:
		type = ff::INPUT_DEVICE_POINTER;
		break;

	case PT_TOUCH:
		type = ff::INPUT_DEVICE_TOUCH;
		break;

	case PT_PEN:
		type = ff::INPUT_DEVICE_PEN;
		break;

	case PT_MOUSE:
		type = ff::INPUT_DEVICE_MOUSE;
		break;

	case PT_TOUCHPAD:
		type = ff::INPUT_DEVICE_TOUCHPAD;
		break;
	}

	if (IS_POINTER_FIRSTBUTTON_WPARAM(wp))
	{
		vk = VK_LBUTTON;
	}
	else if (IS_POINTER_SECONDBUTTON_WPARAM(wp))
	{
		vk = VK_RBUTTON;
	}
	else if (IS_POINTER_THIRDBUTTON_WPARAM(wp))
	{
		vk = VK_MBUTTON;
	}
	else if (IS_POINTER_FOURTHBUTTON_WPARAM(wp))
	{
		vk = VK_XBUTTON1;
	}
	else if (IS_POINTER_FIFTHBUTTON_WPARAM(wp))
	{
		vk = VK_XBUTTON2;
	}

	info.info.type = type;
	info.info.id = id;
	info.info.vk = vk;
	info.info.pos = pos.ToType<double>();
}

#endif // !METRO_APP
