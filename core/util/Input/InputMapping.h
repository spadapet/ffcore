#pragma once

namespace ff
{
	class IJoystickDevice;
	class IKeyboardDevice;
	class IPointerDevice;

	// Which device is being used?
	enum InputDevice : unsigned char
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_DEVICE_NULL,
		INPUT_DEVICE_KEYBOARD,
		INPUT_DEVICE_MOUSE,
		INPUT_DEVICE_JOYSTICK,
		INPUT_DEVICE_TOUCH,
		INPUT_DEVICE_TOUCHPAD,
		INPUT_DEVICE_PEN,
		INPUT_DEVICE_POINTER, // generic
	};

	// What is the user touching on the device?
	enum InputPart : unsigned char
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_PART_NULL,
		INPUT_PART_BUTTON, // keyboard, mouse, or joystick
		INPUT_PART_TEXT, // on keyboard
		INPUT_PART_KEY_BUTTON, // joystick button using VK code
		INPUT_PART_STICK, // joystick
		INPUT_PART_DPAD, // joystick
		INPUT_PART_TRIGGER, // joystick
	};

	enum InputPartValue : unsigned char
	{
		// NOTE: These are persisted, so don't change their order/values

		INPUT_VALUE_DEFAULT,
		INPUT_VALUE_X_AXIS, // joystick
		INPUT_VALUE_Y_AXIS, // joystick
		INPUT_VALUE_LEFT, // joystick
		INPUT_VALUE_RIGHT, // joystick
		INPUT_VALUE_UP, // joystick
		INPUT_VALUE_DOWN, // joystick
	};

	// A single thing that the user can press
	struct InputAction
	{
		UTIL_API bool IsValid() const;
		UTIL_API bool operator==(const InputAction& rhs) const;
		UTIL_API bool operator<(const InputAction& rhs) const;

		InputDevice _device;
		InputPart _part;
		InputPartValue _partValue;
		BYTE _partIndex; // which button or stick?
	};

	// Maps actions to an event
	struct InputEventMapping
	{
		InputAction _actions[4]; // up to four (unused actions must be zeroed out)
		hash_t _eventID;
		double _holdSeconds;
		double _repeatSeconds;
	};

	// Allows direct access to any input value
	struct InputValueMapping
	{
		InputAction _action;
		hash_t _valueID;
	};

	// Sent whenever an InputEventMapping gets triggered (or released)
	struct InputEvent
	{
		hash_t _eventID;
		int _count;

		bool IsStart() const { return _count == 1; }
		bool IsRepeat() const { return _count > 1; }
		bool IsStop() const { return _count == 0; }
	};

	struct InputDevices
	{
		UTIL_API static const InputDevices& Null();

		ff::Vector<ff::ComPtr<ff::IKeyboardDevice>> _keys;
		ff::Vector<ff::ComPtr<ff::IPointerDevice>> _mice;
		ff::Vector<ff::ComPtr<ff::IJoystickDevice>> _joys;
	};

	class IInputEvents
	{
	public:
		virtual const Vector<InputEvent>& GetEvents() const = 0;
		virtual float GetEventProgress(hash_t eventID) const = 0; // 1=triggered once, 2=hold time hit twice, etc...
		virtual bool HasStartEvent(hash_t eventID) const = 0;

		// Gets immediate values
		virtual int GetDigitalValue(const InputDevices& devices, hash_t valueID) const = 0;
		virtual float GetAnalogValue(const InputDevices& devices, hash_t valueID) const = 0;
		virtual String GetStringValue(const InputDevices& devices, hash_t valueID) const = 0;
	};

	class IInputEventsProvider
	{
	public:
		virtual const IInputEvents* GetInputEvents() = 0;
		virtual const InputDevices& GetInputDevices() = 0;
	};

	class __declspec(uuid("53800686-1b66-4256-836a-bdce1b1b2f0b")) __declspec(novtable)
		IInputMapping : public IUnknown, public IInputEvents
	{
	public:
		virtual bool Advance(const InputDevices& devices, double deltaTime) = 0;

		virtual bool MapEvents(const InputEventMapping* pMappings, size_t nCount) = 0;
		virtual bool MapValues(const InputValueMapping* pMappings, size_t nCount) = 0;

		virtual Vector<InputEventMapping> GetMappedEvents() const = 0;
		virtual Vector<InputValueMapping> GetMappedValues() const = 0;

		virtual Vector<InputEventMapping> GetMappedEvents(hash_t eventID) const = 0;
		virtual Vector<InputValueMapping> GetMappedValues(hash_t valueID) const = 0;
	};

	UTIL_API int NameToVirtualKey(StringRef name);
	UTIL_API String VirtualKeyToName(int vk);
	UTIL_API String VirtualKeyToDisplayName(int vk);
	UTIL_API InputAction VirtualKeyToInputAction(int vk);
	UTIL_API InputAction NameToInputAction(StringRef name);
	UTIL_API int InputActionToVirtualKey(const InputAction& action);
	UTIL_API String InputActionToName(const InputAction& action);
	UTIL_API String InputActionToDisplayName(const InputAction& action);
}
