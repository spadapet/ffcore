#include "pch.h"
#include "COM/ComAlloc.h"
#include "Data/Data.h"
#include "Data/DataPersist.h"
#include "Data/DataWriterReader.h"
#include "Dict/Dict.h"
#include "Globals/ProcessGlobals.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Keyboard/KeyboardDevice.h"
#include "Input/Pointer/PointerDevice.h"
#include "Module/ModuleFactory.h"
#include "Resource/ResourcePersist.h"
#include "Resource/Resources.h"
#include "String/StringUtil.h"
#include "Value/Values.h"

static ff::StaticString PROP_ACTION(L"action");
static ff::StaticString PROP_DATA(L"data");
static ff::StaticString PROP_EVENTS(L"events");
static ff::StaticString PROP_HOLD(L"hold");
static ff::StaticString PROP_REPEAT(L"repeat");
static ff::StaticString PROP_VALUES(L"values");
static const ff::InputDevices NULL_INPUT_DEVICES;

class __declspec(uuid("d1dee57f-337b-49d6-9608-c69bbbce4367"))
	InputMapping
	: public ff::ComBase
	, public ff::IInputMapping
	, public ff::IResourcePersist
	, public ff::IResourceGetClone
{
public:
	DECLARE_HEADER(InputMapping);

	// IInputMapping
	virtual bool Advance(const ff::InputDevices& devices, double deltaTime) override;
	virtual bool MapEvents(const ff::InputEventMapping* pMappings, size_t nCount) override;
	virtual bool MapValues(const ff::InputValueMapping* pMappings, size_t nCount) override;
	virtual ff::Vector<ff::InputEventMapping> GetMappedEvents() const override;
	virtual ff::Vector<ff::InputValueMapping> GetMappedValues() const override;
	virtual ff::Vector<ff::InputEventMapping> GetMappedEvents(ff::hash_t eventID) const override;
	virtual ff::Vector<ff::InputValueMapping> GetMappedValues(ff::hash_t valueID) const override;

	// IInputEvents
	virtual const ff::Vector<ff::InputEvent>& GetEvents() const override;
	virtual float GetEventProgress(ff::hash_t eventID) const override;
	virtual bool HasStartEvent(ff::hash_t eventID) const override;
	virtual int GetDigitalValue(const ff::InputDevices& devices, ff::hash_t valueID) const override;
	virtual float GetAnalogValue(const ff::InputDevices& devices, ff::hash_t valueID) const override;
	virtual ff::String GetStringValue(const ff::InputDevices& devices, ff::hash_t valueID) const override;

	// IResourcePersist
	virtual bool LoadFromSource(const ff::Dict& dict) override;
	virtual bool LoadFromCache(const ff::Dict& dict) override;
	virtual bool SaveToCache(ff::Dict& dict) override;

	// IResourceGetClone
	virtual ff::ComPtr<IUnknown> GetResourceClone() override;

private:
	int GetDigitalValue(const ff::InputDevices& devices, const ff::InputAction& action, int* pPressCount) const;
	float GetAnalogValue(const ff::InputDevices& devices, const ff::InputAction& action, bool bForDigital) const;
	ff::String GetStringValue(const ff::InputDevices& devices, const ff::InputAction& action) const;

	struct InputEventMappingInfo : public ff::InputEventMapping
	{
		double _holdingSeconds;
		int _eventCount;
		bool _holding;
	};

	void PushStartEvent(InputEventMappingInfo& info);
	void PushStopEvent(InputEventMappingInfo& info);

	ff::Vector<InputEventMappingInfo> _eventMappings;
	ff::Vector<ff::InputValueMapping> _valueMappings;
	ff::Vector<ff::InputEvent> _currentEvents;

	ff::Map<ff::hash_t, size_t> _eventToInfo;
	ff::Map<ff::hash_t, size_t> _valueToInfo;
};

BEGIN_INTERFACES(InputMapping)
	HAS_INTERFACE(ff::IInputMapping)
	HAS_INTERFACE(ff::IResourcePersist)
	HAS_INTERFACE(ff::IResourceGetClone)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module& module)
	{
		module.RegisterClassT<InputMapping>(L"input");
	});

const ff::InputDevices& ff::InputDevices::Null()
{
	return ::NULL_INPUT_DEVICES;
}

InputMapping::InputMapping()
{
}

InputMapping::~InputMapping()
{
}

bool InputMapping::Advance(const ff::InputDevices& devices, double deltaTime)
{
	_currentEvents.Clear();

	for (size_t i = 0; i < _eventMappings.Size(); i++)
	{
		InputEventMappingInfo& info = _eventMappings[i];

		bool bStillHolding = true;
		int nTriggerCount = 0;

		// Check each required button that needs to be pressed to trigger the current action
		for (size_t h = 0; h < _countof(info._actions) && info._actions[h]._device != ff::INPUT_DEVICE_NULL; h++)
		{
			int nCurTriggerCount = 0;
			int nCurValue = GetDigitalValue(devices, info._actions[h], &nCurTriggerCount);

			if (!nCurValue)
			{
				bStillHolding = false;
				nTriggerCount = 0;
			}

			if (bStillHolding && nCurTriggerCount)
			{
				nTriggerCount = std::max(nTriggerCount, nCurTriggerCount);
			}
		}

		for (int h = 0; h < nTriggerCount; h++)
		{
			if (info._eventCount > 0)
			{
				PushStopEvent(info);
			}

			info._holding = true;

			if (deltaTime / nTriggerCount >= info._holdSeconds)
			{
				PushStartEvent(info);
			}
		}

		if (info._holding)
		{
			if (!bStillHolding)
			{
				PushStopEvent(info);
			}
			else if (!nTriggerCount)
			{
				info._holdingSeconds += deltaTime;

				double holdTime = (info._holdingSeconds - info._holdSeconds);

				if (holdTime >= 0)
				{
					int totalEvents = 1;

					if (info._repeatSeconds > 0)
					{
						totalEvents += (int)floor(holdTime / info._repeatSeconds);
					}

					while (info._eventCount < totalEvents)
					{
						PushStartEvent(info);
					}
				}
			}
		}
	}

#if !METRO_APP
	if (_currentEvents.Size())
	{
		// The user is doing something, tell Windows not to go to sleep (mostly for joysticks)
		SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);
	}
#endif

	return !_currentEvents.IsEmpty();
}

bool InputMapping::MapEvents(const ff::InputEventMapping* pMappings, size_t nCount)
{
	bool status = true;

	_eventMappings.Reserve(_eventMappings.Size() + nCount);

	for (size_t i = 0; i < nCount; i++)
	{
		InputEventMappingInfo info;
		ZeroObject(info);

		::CopyMemory(&info, &pMappings[i], sizeof(pMappings[i]));

		_eventMappings.Push(info);
		_eventToInfo.InsertKey(info._eventID, _eventMappings.Size() - 1);
	}

	return status;
}

bool InputMapping::MapValues(const ff::InputValueMapping* pMappings, size_t nCount)
{
	bool status = true;

	_valueMappings.Reserve(_valueMappings.Size() + nCount);

	for (size_t i = 0; i < nCount; i++)
	{
		_valueMappings.Push(pMappings[i]);
		_valueToInfo.InsertKey(pMappings[i]._valueID, _valueMappings.Size() - 1);
	}

	return status;
}

ff::Vector<ff::InputEventMapping> InputMapping::GetMappedEvents() const
{
	ff::Vector<ff::InputEventMapping> mappings;
	mappings.Reserve(_eventMappings.Size());

	for (size_t i = 0; i < _eventMappings.Size(); i++)
	{
		mappings.Push(_eventMappings[i]);
	}

	return mappings;
}

ff::Vector<ff::InputValueMapping> InputMapping::GetMappedValues() const
{
	return _valueMappings;
}

ff::Vector<ff::InputEventMapping> InputMapping::GetMappedEvents(ff::hash_t eventID) const
{
	ff::Vector<ff::InputEventMapping> mappings;

	for (auto i = _eventToInfo.GetKey(eventID); i; i = _eventToInfo.GetNextDupeKey(*i))
	{
		mappings.Push(_eventMappings[i->GetValue()]);
	}

	return mappings;
}

ff::Vector<ff::InputValueMapping> InputMapping::GetMappedValues(ff::hash_t valueID) const
{
	ff::Vector<ff::InputValueMapping> mappings;

	for (auto i = _valueToInfo.GetKey(valueID); i; i = _valueToInfo.GetNextDupeKey(*i))
	{
		mappings.Push(_valueMappings[i->GetValue()]);
	}

	return mappings;
}

const ff::Vector<ff::InputEvent>& InputMapping::GetEvents() const
{
	return _currentEvents;
}

float InputMapping::GetEventProgress(ff::hash_t eventID) const
{
	double ret = 0;

	for (auto i = _eventToInfo.GetKey(eventID); i; i = _eventToInfo.GetNextDupeKey(*i))
	{
		const InputEventMappingInfo& info = _eventMappings[i->GetValue()];

		if (info._holding)
		{
			double val = 1;

			if (info._holdingSeconds < info._holdSeconds)
			{
				// from 0 to 1
				val = info._holdingSeconds / info._holdSeconds;
			}
			else if (info._repeatSeconds > 0)
			{
				// from 1 to N, using repeat count
				val = (info._holdingSeconds - info._holdSeconds) / info._repeatSeconds + 1.0;
			}
			else if (info._holdSeconds > 0)
			{
				// from 1 to N, using the original hold time as the repeat time
				val = info._holdingSeconds / info._holdSeconds;
			}

			// Can only return one value, so choose the largest
			if (val > ret)
			{
				ret = val;
			}
		}
	}

	return (float)ret;
}

bool InputMapping::HasStartEvent(ff::hash_t eventID) const
{
	for (const ff::InputEvent& event : _currentEvents)
	{
		if (event._eventID == eventID && event._count >= 1)
		{
			return true;
		}
	}

	return false;
}

int InputMapping::GetDigitalValue(const ff::InputDevices& devices, ff::hash_t valueID) const
{
	int ret = 0;

	for (auto i = _valueToInfo.GetKey(valueID); i; i = _valueToInfo.GetNextDupeKey(*i))
	{
		const ff::InputAction& action = _valueMappings[i->GetValue()]._action;

		int val = GetDigitalValue(devices, action, nullptr);

		// Can only return one value, so choose the largest
		if (abs(val) > abs(ret))
		{
			ret = val;
		}
	}

	return ret;
}

float InputMapping::GetAnalogValue(const ff::InputDevices& devices, ff::hash_t valueID) const
{
	float ret = 0;

	for (auto i = _valueToInfo.GetKey(valueID); i; i = _valueToInfo.GetNextDupeKey(*i))
	{
		const ff::InputAction& action = _valueMappings[i->GetValue()]._action;

		float val = GetAnalogValue(devices, action, false);

		// Can only return one value, so choose the largest
		if (fabsf(val) > fabsf(ret))
		{
			ret = val;
		}
	}

	return ret;
}

ff::String InputMapping::GetStringValue(const ff::InputDevices& devices, ff::hash_t valueID) const
{
	ff::String ret;

	for (auto i = _valueToInfo.GetKey(valueID); i; i = _valueToInfo.GetNextDupeKey(*i))
	{
		const ff::InputAction& action = _valueMappings[i->GetValue()]._action;

		ret += GetStringValue(devices, action);
	}

	return ret;
}

static bool ParseEventMappings(const ff::Dict& dict, ff::Vector<ff::InputEventMapping>& mappings, bool forValues)
{
	for (ff::String name : dict.GetAllNames())
	{
		ff::ValuePtr rootValue = dict.GetValue(name);
		assertRetVal(rootValue->IsType<ff::ValueVectorValue>(), false);

		for (ff::ValuePtr nestedValue : rootValue->GetValue<ff::ValueVectorValue>())
		{
			ff::InputEventMapping mapping;
			ff::ZeroObject(mapping);
			mapping._eventID = ff::HashFunc(name);

			assertRetVal(nestedValue->IsType<ff::DictValue>(), false);
			const ff::Dict& nestedDict = nestedValue->GetValue<ff::DictValue>();

			if (!forValues)
			{
				mapping._holdSeconds = nestedDict.Get<ff::DoubleValue>(PROP_HOLD);
				mapping._repeatSeconds = nestedDict.Get<ff::DoubleValue>(PROP_REPEAT);
			}

			ff::Vector<ff::InputAction> actions;
			ff::ValuePtr actionValue = nestedDict.GetValue(PROP_ACTION);

			if (forValues)
			{
				assertRetVal(actionValue->IsType<ff::StringValue>(), false);
				ff::InputAction action = ff::NameToInputAction(actionValue->GetValue<ff::StringValue>());
				assertRetVal(action.IsValid(), false);
				actions.Push(action);
			}
			else
			{
				assertRetVal(actionValue->IsType<ff::ValueVectorValue>(), false);
				for (ff::ValuePtr singleActionValue : actionValue->GetValue<ff::ValueVectorValue>())
				{
					assertRetVal(singleActionValue->IsType<ff::StringValue>(), false);
					ff::InputAction action = ff::NameToInputAction(singleActionValue->GetValue<ff::StringValue>());
					assertRetVal(action.IsValid(), false);
					actions.Push(action);
				}
			}

			assertRetVal(actions.Size() > 0 && actions.Size() < _countof(mapping._actions), false);
			memcpy(mapping._actions, actions.Data(), actions.ByteSize());
			mappings.Push(mapping);
		}
	}

	return true;
}

static ff::Vector<ff::InputValueMapping> MapEventsToValues(const ff::Vector<ff::InputEventMapping>& mappings)
{
	ff::Vector<ff::InputValueMapping> result;

	for (const ff::InputEventMapping& mapping : mappings)
	{
		ff::InputValueMapping mapping2;
		mapping2._action = mapping._actions[0];
		mapping2._valueID = mapping._eventID;
		result.Push(mapping2);
	}

	return result;
}

bool InputMapping::LoadFromSource(const ff::Dict& dict)
{
	ff::Dict eventsDict = dict.Get<ff::DictValue>(PROP_EVENTS);
	ff::Dict valuesDict = dict.Get<ff::DictValue>(PROP_VALUES);

	ff::Vector<ff::InputEventMapping> eventMappings;
	ff::Vector<ff::InputEventMapping> valueMappings;
	assertRetVal(::ParseEventMappings(eventsDict, eventMappings, false), nullptr);
	assertRetVal(::ParseEventMappings(valuesDict, valueMappings, true), nullptr);

	if (eventMappings.Size())
	{
		MapEvents(eventMappings.Data(), eventMappings.Size());
	}

	if (valueMappings.Size())
	{
		ff::Vector<ff::InputValueMapping> valueMappings2 = ::MapEventsToValues(valueMappings);
		MapValues(valueMappings2.Data(), valueMappings2.Size());
	}

	return true;
}

bool InputMapping::LoadFromCache(const ff::Dict& dict)
{
	ff::ComPtr<ff::IData> data = dict.Get<ff::DataValue>(PROP_DATA);
	noAssertRetVal(data, true);

	ff::ComPtr<ff::IDataReader> reader;
	assertRetVal(CreateDataReader(data, 0, &reader), false);

	DWORD version = 0;
	assertRetVal(LoadData(reader, version), false);
	assertRetVal(version == 0, false);

	DWORD size = 0;
	assertRetVal(LoadData(reader, size), false);

	if (size)
	{
		ff::Vector<ff::InputEventMapping> mappings;
		mappings.Resize(size);

		assertRetVal(LoadBytes(reader, mappings.Data(), mappings.ByteSize()), false);
		assertRetVal(MapEvents(mappings.Data(), mappings.Size()), false);
	}

	assertRetVal(LoadData(reader, size), false);

	if (size)
	{
		ff::Vector<ff::InputValueMapping> mappings;
		mappings.Resize(size);

		assertRetVal(LoadBytes(reader, mappings.Data(), mappings.ByteSize()), false);
		assertRetVal(MapValues(mappings.Data(), mappings.Size()), false);
	}

	return true;
}

bool InputMapping::SaveToCache(ff::Dict& dict)
{
	ff::ComPtr<ff::IDataVector> data;
	ff::ComPtr<ff::IDataWriter> writer;
	assertRetVal(CreateDataWriter(&data, &writer), false);

	DWORD version = 0;
	assertRetVal(SaveData(writer, version), false);

	DWORD size = (DWORD)_eventMappings.Size();
	assertRetVal(SaveData(writer, size), false);

	for (size_t i = 0; i < size; i++)
	{
		ff::InputEventMapping info = _eventMappings[i];
		assertRetVal(SaveData(writer, info), false);
	}

	size = (DWORD)_valueMappings.Size();
	assertRetVal(SaveData(writer, size), false);

	if (size)
	{
		assertRetVal(SaveBytes(writer, _valueMappings.Data(), _valueMappings.ByteSize()), false);
	}

	dict.Set<ff::DataValue>(PROP_DATA, data);

	return true;
}

ff::ComPtr<IUnknown> InputMapping::GetResourceClone()
{
	ff::ComPtr<ff::IInputMapping> myClone = new ff::ComObject<InputMapping>();
	ff::Vector<ff::InputEventMapping> events = GetMappedEvents();
	ff::Vector<ff::InputValueMapping> values = GetMappedValues();

	if (events.Size())
	{
		assertRetVal(myClone->MapEvents(events.Data(), events.Size()), false);
	}

	if (values.Size())
	{
		assertRetVal(myClone->MapValues(values.Data(), values.Size()), false);
	}

	ff::ComPtr<IUnknown> unknownClone;
	unknownClone.QueryFrom(myClone);
	return unknownClone;
}

int InputMapping::GetDigitalValue(const ff::InputDevices& devices, const ff::InputAction& action, int* pPressCount) const
{
	int nFakePressCount;
	pPressCount = pPressCount ? pPressCount : &nFakePressCount;
	*pPressCount = 0;

	switch (action._device)
	{
	case ff::INPUT_DEVICE_KEYBOARD:
		if (action._part == ff::INPUT_PART_BUTTON)
		{
			int nRet = 0;

			for (size_t i = 0; i < devices._keys.Size(); i++)
			{
				*pPressCount += devices._keys[i]->GetKeyPressCount(action._partIndex);

				if (devices._keys[i]->GetKey(action._partIndex))
				{
					nRet = 1;
				}
			}

			return nRet;
		}
		else if (action._part == ff::INPUT_PART_TEXT)
		{
			for (size_t i = 0; i < devices._keys.Size(); i++)
			{
				if (devices._keys[i]->GetChars().size())
				{
					*pPressCount = 1;
					return 1;
				}
			}
		}
		break;

	case ff::INPUT_DEVICE_MOUSE:
		if (action._part == ff::INPUT_PART_BUTTON)
		{
			int nRet = 0;

			for (size_t i = 0; i < devices._mice.Size(); i++)
			{
				*pPressCount += devices._mice[i]->GetButtonClickCount(action._partIndex);

				if (devices._mice[i]->GetButton(action._partIndex))
				{
					nRet = 1;
				}
			}

			return nRet;
		}
		break;

	case ff::INPUT_DEVICE_JOYSTICK:
		if (action._part == ff::INPUT_PART_BUTTON)
		{
			int nRet = 0;

			for (size_t i = 0; i < devices._joys.Size(); i++)
			{
				if (devices._joys[i]->IsConnected())
				{
					size_t nButtonCount = devices._joys[i]->GetButtonCount();
					bool bAllButtons = (action._partIndex == 0xFF);

					if (bAllButtons || action._partIndex < nButtonCount)
					{
						size_t nFirst = bAllButtons ? 0 : action._partIndex;
						size_t nEnd = bAllButtons ? nButtonCount : nFirst + 1;

						for (size_t h = nFirst; h < nEnd; h++)
						{
							*pPressCount += devices._joys[i]->GetButtonPressCount(h);

							if (devices._joys[i]->GetButton(h))
							{
								nRet = 1;
							}
						}
					}
				}
			}

			return nRet;
		}
		else if (action._part == ff::INPUT_PART_KEY_BUTTON)
		{
			int nRet = 0;
			int vk = action._partIndex;

			for (size_t i = 0; i < devices._joys.Size(); i++)
			{
				if (devices._joys[i]->IsConnected() && devices._joys[i]->HasKeyButton(vk))
				{
					*pPressCount += devices._joys[i]->GetKeyButtonPressCount(vk);

					if (devices._joys[i]->GetKeyButton(vk))
					{
						nRet = 1;
					}
				}
			}

			return nRet;
		}
		else if (action._part == ff::INPUT_PART_TRIGGER)
		{
			for (size_t i = 0; i < devices._joys.Size(); i++)
			{
				if (devices._joys[i]->IsConnected() && action._partIndex < devices._joys[i]->GetTriggerCount())
				{
					*pPressCount += devices._joys[i]->GetTriggerPressCount(action._partIndex);
				}
			}

			return GetAnalogValue(devices, action, true) >= 0.5f ? 1 : 0;
		}
		else if (action._part == ff::INPUT_PART_STICK)
		{
			if (action._partValue == ff::INPUT_VALUE_LEFT ||
				action._partValue == ff::INPUT_VALUE_RIGHT ||
				action._partValue == ff::INPUT_VALUE_UP ||
				action._partValue == ff::INPUT_VALUE_DOWN)
			{
				for (size_t i = 0; i < devices._joys.Size(); i++)
				{
					if (devices._joys[i]->IsConnected() && action._partIndex < devices._joys[i]->GetStickCount())
					{
						ff::RectInt dirs = devices._joys[i]->GetStickPressCount(action._partIndex);

						switch (action._partValue)
						{
						case ff::INPUT_VALUE_LEFT: *pPressCount += dirs.left; break;
						case ff::INPUT_VALUE_RIGHT: *pPressCount += dirs.right; break;
						case ff::INPUT_VALUE_UP: *pPressCount += dirs.top; break;
						case ff::INPUT_VALUE_DOWN: *pPressCount += dirs.bottom; break;
						}
					}
				}
			}

			return GetAnalogValue(devices, action, true) >= 0.5f ? 1 : 0;
		}
		else if (action._part == ff::INPUT_PART_DPAD)
		{
			if (action._partValue == ff::INPUT_VALUE_LEFT ||
				action._partValue == ff::INPUT_VALUE_RIGHT ||
				action._partValue == ff::INPUT_VALUE_UP ||
				action._partValue == ff::INPUT_VALUE_DOWN)
			{
				for (size_t i = 0; i < devices._joys.Size(); i++)
				{
					if (devices._joys[i]->IsConnected() && action._partIndex < devices._joys[i]->GetDPadCount())
					{
						ff::RectInt dirs = devices._joys[i]->GetDPadPressCount(action._partIndex);

						switch (action._partValue)
						{
						case ff::INPUT_VALUE_LEFT: *pPressCount += dirs.left; break;
						case ff::INPUT_VALUE_RIGHT: *pPressCount += dirs.right; break;
						case ff::INPUT_VALUE_UP: *pPressCount += dirs.top; break;
						case ff::INPUT_VALUE_DOWN: *pPressCount += dirs.bottom; break;
						}
					}
				}
			}

			return GetAnalogValue(devices, action, true) >= 0.5f ? 1 : 0;
		}
		break;
	}

	return 0;
}

float InputMapping::GetAnalogValue(const ff::InputDevices& devices, const ff::InputAction& action, bool bForDigital) const
{
	switch (action._device)
	{
	case ff::INPUT_DEVICE_KEYBOARD:
	case ff::INPUT_DEVICE_MOUSE:
		return GetDigitalValue(devices, action, nullptr) ? 1.0f : 0.0f;

	case ff::INPUT_DEVICE_JOYSTICK:
		if (action._part == ff::INPUT_PART_BUTTON)
		{
			return GetDigitalValue(devices, action, nullptr) ? 1.0f : 0.0f;
		}
		else if (action._part == ff::INPUT_PART_TRIGGER)
		{
			float val = 0;

			for (size_t i = 0; i < devices._joys.Size(); i++)
			{
				if (devices._joys[i]->IsConnected() && action._partIndex < devices._joys[i]->GetTriggerCount())
				{
					float newVal = devices._joys[i]->GetTrigger(action._partIndex, bForDigital);

					if (fabsf(newVal) > fabsf(val))
					{
						val = newVal;
					}
				}
			}

			return val;
		}
		else if (action._part == ff::INPUT_PART_STICK)
		{
			if (action._partValue == ff::INPUT_VALUE_X_AXIS ||
				action._partValue == ff::INPUT_VALUE_Y_AXIS ||
				action._partValue == ff::INPUT_VALUE_LEFT ||
				action._partValue == ff::INPUT_VALUE_RIGHT ||
				action._partValue == ff::INPUT_VALUE_UP ||
				action._partValue == ff::INPUT_VALUE_DOWN)
			{
				ff::PointFloat posXY(0, 0);

				for (size_t i = 0; i < devices._joys.Size(); i++)
				{
					if (devices._joys[i]->IsConnected() && action._partIndex < devices._joys[i]->GetStickCount())
					{
						ff::PointFloat newXY = devices._joys[i]->GetStickPos(action._partIndex, bForDigital);

						if (fabsf(newXY.x) > fabsf(posXY.x))
						{
							posXY.x = newXY.x;
						}

						if (fabsf(newXY.y) > fabsf(posXY.y))
						{
							posXY.y = newXY.y;
						}
					}
				}

				switch (action._partValue)
				{
				case ff::INPUT_VALUE_X_AXIS: return posXY.x;
				case ff::INPUT_VALUE_Y_AXIS: return posXY.y;
				case ff::INPUT_VALUE_LEFT: return (posXY.x < 0) ? -posXY.x : 0.0f;
				case ff::INPUT_VALUE_RIGHT: return (posXY.x > 0) ? posXY.x : 0.0f;
				case ff::INPUT_VALUE_UP: return (posXY.y < 0) ? -posXY.y : 0.0f;
				case ff::INPUT_VALUE_DOWN: return (posXY.y > 0) ? posXY.y : 0.0f;
				}
			}
		}
		else if (action._part == ff::INPUT_PART_DPAD)
		{
			if (action._partValue == ff::INPUT_VALUE_X_AXIS ||
				action._partValue == ff::INPUT_VALUE_Y_AXIS ||
				action._partValue == ff::INPUT_VALUE_LEFT ||
				action._partValue == ff::INPUT_VALUE_RIGHT ||
				action._partValue == ff::INPUT_VALUE_UP ||
				action._partValue == ff::INPUT_VALUE_DOWN)
			{
				ff::PointInt posXY(0, 0);

				for (size_t i = 0; i < devices._joys.Size(); i++)
				{
					if (devices._joys[i]->IsConnected() && action._partIndex < devices._joys[i]->GetDPadCount())
					{
						ff::PointInt newXY = devices._joys[i]->GetDPadPos(action._partIndex);

						if (abs(newXY.x) > abs(posXY.x))
						{
							posXY.x = newXY.x;
						}

						if (abs(newXY.y) > abs(posXY.y))
						{
							posXY.y = newXY.y;
						}
					}
				}

				switch (action._partValue)
				{
				case ff::INPUT_VALUE_X_AXIS: return (float)posXY.x;
				case ff::INPUT_VALUE_Y_AXIS: return (float)posXY.y;
				case ff::INPUT_VALUE_LEFT: return (posXY.x < 0) ? 1.0f : 0.0f;
				case ff::INPUT_VALUE_RIGHT: return (posXY.x > 0) ? 1.0f : 0.0f;
				case ff::INPUT_VALUE_UP: return (posXY.y < 0) ? 1.0f : 0.0f;
				case ff::INPUT_VALUE_DOWN: return (posXY.y > 0) ? 1.0f : 0.0f;
				}
			}
		}
		break;
	}

	return 0;
}

ff::String InputMapping::GetStringValue(const ff::InputDevices& devices, const ff::InputAction& action) const
{
	ff::String szText;

	if (action._device == ff::INPUT_DEVICE_KEYBOARD && action._part == ff::INPUT_PART_TEXT)
	{
		for (size_t i = 0; i < devices._keys.Size(); i++)
		{
			if (szText.empty())
			{
				szText = devices._keys[i]->GetChars();
			}
			else
			{
				szText += devices._keys[i]->GetChars();
			}
		}

		return szText;
	}

	return szText;
}

void InputMapping::PushStartEvent(InputEventMappingInfo& info)
{
	ff::InputEvent ev;
	ev._eventID = info._eventID;
	ev._count = ++info._eventCount;

	_currentEvents.Push(ev);
}

void InputMapping::PushStopEvent(InputEventMappingInfo& info)
{
	bool bEvent = (info._eventCount > 0);

	info._holdingSeconds = 0;
	info._eventCount = 0;
	info._holding = false;

	if (bEvent)
	{
		ff::InputEvent ev;
		ev._eventID = info._eventID;
		ev._count = 0;

		_currentEvents.Push(ev);
	}
}

// Big static table of all virtual keys
struct VirtualKeyInfo
{
	int vk;
	ff::StaticString name;
	ff::InputAction action;
};

static const VirtualKeyInfo s_vksRaw[] =
{
	{ VK_LBUTTON, { L"mouse-left" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_LBUTTON } },
	{ VK_RBUTTON, { L"mouse-right" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_RBUTTON } },
	{ VK_MBUTTON, { L"mouse-middle" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_MBUTTON } },
	{ VK_XBUTTON1, { L"mouse-x1" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_XBUTTON1 } },
	{ VK_XBUTTON2, { L"mouse-x2" }, { ff::INPUT_DEVICE_MOUSE, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_XBUTTON2 } },
	{ VK_BACK, { L"backspace" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_BACK } },
	{ VK_TAB, { L"tab" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_TAB } },
	{ VK_RETURN, { L"return" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_RETURN } },
	{ VK_SHIFT, { L"shift" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_SHIFT } },
	{ VK_CONTROL, { L"control" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_CONTROL } },
	{ VK_MENU, { L"alt" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_MENU } },
	{ VK_ESCAPE, { L"escape" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_ESCAPE } },
	{ VK_SPACE, { L"space" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_SPACE } },
	{ VK_PRIOR, { L"pageup" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_PRIOR } },
	{ VK_NEXT, { L"pagedown" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NEXT } },
	{ VK_END, { L"end" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_END } },
	{ VK_HOME, { L"home" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_HOME } },
	{ VK_LEFT, { L"left" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_LEFT } },
	{ VK_UP, { L"up" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_UP } },
	{ VK_RIGHT, { L"right" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_RIGHT } },
	{ VK_DOWN, { L"down" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_DOWN } },
	{ VK_SNAPSHOT, { L"printscreen" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_SNAPSHOT } },
	{ VK_INSERT, { L"insert" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_INSERT } },
	{ VK_DELETE, { L"delete" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_DELETE } },
	{ '0', { L"0" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '0' } },
	{ '1', { L"1" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '1' } },
	{ '2', { L"2" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '2' } },
	{ '3', { L"3" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '3' } },
	{ '4', { L"4" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '4' } },
	{ '5', { L"5" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '5' } },
	{ '6', { L"6" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '6' } },
	{ '7', { L"7" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '7' } },
	{ '8', { L"8" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '8' } },
	{ '9', { L"9" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, '9' } },
	{ 'A', { L"a" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'A' } },
	{ 'B', { L"b" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'B' } },
	{ 'C', { L"c" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'C' } },
	{ 'D', { L"d" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'D' } },
	{ 'E', { L"e" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'E' } },
	{ 'F', { L"f" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'F' } },
	{ 'G', { L"g" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'G' } },
	{ 'H', { L"h" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'H' } },
	{ 'I', { L"i" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'I' } },
	{ 'J', { L"j" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'J' } },
	{ 'K', { L"k" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'K' } },
	{ 'L', { L"l" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'L' } },
	{ 'M', { L"m" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'M' } },
	{ 'N', { L"n" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'N' } },
	{ 'O', { L"o" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'O' } },
	{ 'P', { L"p" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'P' } },
	{ 'Q', { L"q" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'Q' } },
	{ 'R', { L"r" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'R' } },
	{ 'S', { L"s" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'S' } },
	{ 'T', { L"t" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'T' } },
	{ 'U', { L"u" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'U' } },
	{ 'V', { L"v" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'V' } },
	{ 'W', { L"w" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'W' } },
	{ 'X', { L"x" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'X' } },
	{ 'Y', { L"y" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'Y' } },
	{ 'Z', { L"z" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, 'Z' } },
	{ VK_LWIN, { L"lwin" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_LWIN } },
	{ VK_RWIN, { L"rwin" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_RWIN } },
	{ VK_NUMPAD0, { L"num0" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD0 } },
	{ VK_NUMPAD1, { L"num1" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD1 } },
	{ VK_NUMPAD2, { L"num2" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD2 } },
	{ VK_NUMPAD3, { L"num3" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD3 } },
	{ VK_NUMPAD4, { L"num4" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD4 } },
	{ VK_NUMPAD5, { L"num5" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD5 } },
	{ VK_NUMPAD6, { L"num6" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD6 } },
	{ VK_NUMPAD7, { L"num7" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD7 } },
	{ VK_NUMPAD8, { L"num8" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD8 } },
	{ VK_NUMPAD9, { L"num9" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_NUMPAD9 } },
	{ VK_MULTIPLY, { L"multiply" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_MULTIPLY } },
	{ VK_ADD, { L"add" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_ADD } },
	{ VK_SUBTRACT, { L"subtract" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_SUBTRACT } },
	{ VK_DECIMAL, { L"decimal" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_DECIMAL } },
	{ VK_DIVIDE, { L"divide" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_DIVIDE } },
	{ VK_F1, { L"f1" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F1 } },
	{ VK_F2, { L"f2" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F2 } },
	{ VK_F3, { L"f3" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F3 } },
	{ VK_F4, { L"f4" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F4 } },
	{ VK_F5, { L"f5" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F5 } },
	{ VK_F6, { L"f6" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F6 } },
	{ VK_F7, { L"f7" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F7 } },
	{ VK_F8, { L"f8" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F8 } },
	{ VK_F9, { L"f9" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F9 } },
	{ VK_F10, { L"f10" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F10 } },
	{ VK_F11, { L"f11" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F11 } },
	{ VK_F12, { L"f12" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_F12 } },
	{ VK_LSHIFT, { L"lshift" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_LSHIFT } },
	{ VK_RSHIFT, { L"rshift" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_RSHIFT } },
	{ VK_LCONTROL, { L"lcontrol" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_LCONTROL } },
	{ VK_RCONTROL, { L"rcontrol" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_RCONTROL } },
	{ VK_LMENU, { L"lalt" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_LMENU } },
	{ VK_RMENU, { L"ralt" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_RMENU } },
	{ VK_OEM_1, { L"colon" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_1 } },
	{ VK_OEM_PLUS, { L"plus" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_PLUS } },
	{ VK_OEM_COMMA, { L"comma" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_COMMA } },
	{ VK_OEM_MINUS, { L"minus" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_MINUS } },
	{ VK_OEM_PERIOD, { L"period" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_PERIOD } },
	{ VK_OEM_2, { L"question" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_2 } },
	{ VK_OEM_3, { L"tilde" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_3 } },
	{ VK_GAMEPAD_A, { L"joy-a" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_A } },
	{ VK_GAMEPAD_B, { L"joy-b" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_B } },
	{ VK_GAMEPAD_X, { L"joy-x" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_X } },
	{ VK_GAMEPAD_Y, { L"joy-y" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_Y } },
	{ VK_GAMEPAD_RIGHT_SHOULDER, { L"joy-rbumper" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_RIGHT_SHOULDER } },
	{ VK_GAMEPAD_LEFT_SHOULDER, { L"joy-lbumper" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_LEFT_SHOULDER } },
	{ VK_GAMEPAD_LEFT_TRIGGER, { L"joy-ltrigger" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_TRIGGER, ff::INPUT_VALUE_DEFAULT, 0 } },
	{ VK_GAMEPAD_RIGHT_TRIGGER, { L"joy-rtrigger" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_TRIGGER, ff::INPUT_VALUE_DEFAULT, 1 } },
	{ VK_GAMEPAD_DPAD_UP, { L"joy-dup" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_UP, 0 } },
	{ VK_GAMEPAD_DPAD_DOWN, { L"joy-ddown" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_DOWN, 0 } },
	{ VK_GAMEPAD_DPAD_LEFT, { L"joy-dleft" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_LEFT, 0 } },
	{ VK_GAMEPAD_DPAD_RIGHT, { L"joy-dright" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_DPAD, ff::INPUT_VALUE_RIGHT, 0 } },
	{ VK_GAMEPAD_MENU, { L"joy-start" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_MENU } },
	{ VK_GAMEPAD_VIEW, { L"joy-back" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_VIEW } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON, { L"joy-lthumb" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON, { L"joy-rthumb" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_KEY_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_UP, { L"joy-lup" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_UP, 0 } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_DOWN, { L"joy-ldown" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_DOWN, 0 } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT, { L"joy-lright" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_RIGHT, 0 } },
	{ VK_GAMEPAD_LEFT_THUMBSTICK_LEFT, { L"joy-lleft" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_LEFT, 0 } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_UP, { L"joy-rup" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_UP, 1 } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN, { L"joy-rdown" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_DOWN, 1 } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT, { L"joy-rright" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_RIGHT, 1 } },
	{ VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT, { L"joy-rleft" }, { ff::INPUT_DEVICE_JOYSTICK, ff::INPUT_PART_STICK, ff::INPUT_VALUE_LEFT, 1 } },
	{ VK_OEM_4, { L"opencurly" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_4 } },
	{ VK_OEM_5, { L"pipe" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_5 } },
	{ VK_OEM_6, { L"closecurly" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_6 } },
	{ VK_OEM_7, { L"quote" }, { ff::INPUT_DEVICE_KEYBOARD, ff::INPUT_PART_BUTTON, ff::INPUT_VALUE_DEFAULT, VK_OEM_7 } },
};

static bool s_initVirtualKeys;
static const VirtualKeyInfo* s_vkToInfo[0xFF];
static ff::Map<ff::String, const VirtualKeyInfo*> s_nameToInfo;
static ff::Map<ff::InputAction, const VirtualKeyInfo*> s_actionToInfo;

static void InitVirtualKeyInfo()
{
	static ff::Mutex mutex;

	if (!s_initVirtualKeys)
	{
		ff::LockMutex lock(mutex);

		if (!s_initVirtualKeys)
		{
			for (const VirtualKeyInfo& info : s_vksRaw)
			{
				assert(info.vk > 0 && info.vk < _countof(s_vkToInfo));
				assert(info.name.GetString().size());
				assert(!s_vkToInfo[info.vk]);
				assert(!s_nameToInfo.KeyExists(info.name.GetString()));
				assert(!s_actionToInfo.KeyExists(info.action));

				s_vkToInfo[info.vk] = &info;
				s_nameToInfo.SetKey(info.name.GetString(), &info);
				s_actionToInfo.SetKey(info.action, &info);
			}

			ff::AtProgramShutdown([]()
				{
					ff::ZeroObject(s_vkToInfo);
					s_nameToInfo.Clear();
					s_actionToInfo.Clear();
					s_initVirtualKeys = false;
				});

			s_initVirtualKeys = true;
		}
	}
}

int ff::NameToVirtualKey(ff::StringRef name)
{
	InitVirtualKeyInfo();

	auto iter = s_nameToInfo.GetKey(name);
	if (iter)
	{
		return iter->GetValue()->vk;
	}

	assertRetVal(false, 0);
}

ff::String ff::VirtualKeyToName(int vk)
{
	InitVirtualKeyInfo();

	if (vk > 0 && vk < _countof(s_vkToInfo) && s_vkToInfo[vk])
	{
		return s_vkToInfo[vk]->name.GetString();
	}

	assertRetVal(false, ff::GetEmptyString());
}

ff::String ff::VirtualKeyToDisplayName(int vk)
{
	if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9'))
	{
		return ff::String(1, (wchar_t)vk);
	}

	ff::String name = ff::VirtualKeyToName(vk);

	ff::String id = name;
	ff::UpperCaseInPlace(id);
	ff::ReplaceAll(id, '-', '_');
	id.insert(0, L"VK_");

	ff::String displayName = ff::GetThisModule().GetResources()->GetString(id);
	assert(displayName.size());

	return displayName.empty() ? name : displayName;
}

ff::InputAction ff::VirtualKeyToInputAction(int vk)
{
	InitVirtualKeyInfo();

	if (vk > 0 && vk < _countof(s_vkToInfo) && s_vkToInfo[vk])
	{
		return s_vkToInfo[vk]->action;
	}

	ff::InputAction action;
	ff::ZeroObject(action);
	assertRetVal(false, action);
}

ff::InputAction ff::NameToInputAction(ff::StringRef name)
{
	int vk = NameToVirtualKey(name);
	return VirtualKeyToInputAction(vk);
}

int ff::InputActionToVirtualKey(const ff::InputAction& action)
{
	InitVirtualKeyInfo();

	auto iter = s_actionToInfo.GetKey(action);
	if (iter)
	{
		return iter->GetValue()->vk;
	}

	assertRetVal(false, 0);
}

ff::String ff::InputActionToName(const ff::InputAction& action)
{
	int vk = InputActionToVirtualKey(action);
	return VirtualKeyToName(vk);
}

ff::String ff::InputActionToDisplayName(const ff::InputAction& action)
{
	int vk = InputActionToVirtualKey(action);
	return VirtualKeyToDisplayName(vk);
}

bool ff::InputAction::IsValid() const
{
	return _device && _part;
}

bool ff::InputAction::operator==(const ff::InputAction& rhs) const
{
	return std::memcmp(this, &rhs, sizeof(ff::InputAction)) == 0;
}

bool ff::InputAction::operator<(const ff::InputAction& rhs) const
{
	return std::memcmp(this, &rhs, sizeof(ff::InputAction)) < 0;
}
