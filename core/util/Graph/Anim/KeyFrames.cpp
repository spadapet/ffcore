#include "pch.h"
#include "Dict/Dict.h"
#include "Graph/Anim/KeyFrames.h"
#include "Value/Values.h"

static ff::StaticString PROP_DEFAULT(L"default");
static ff::StaticString PROP_FRAME(L"frame");
static ff::StaticString PROP_FRAMES(L"frames");
static ff::StaticString PROP_LENGTH(L"length");
static ff::StaticString PROP_LOOP(L"loop");
static ff::StaticString PROP_METHOD(L"method");
static ff::StaticString PROP_NAME(L"name");
static ff::StaticString PROP_START(L"start");
static ff::StaticString PROP_TANGENTS(L"tangents");
static ff::StaticString PROP_TENSION(L"tension");
static ff::StaticString PROP_VALUE(L"value");
static ff::StaticString PROP_VALUES(L"values");

bool ff::KeyFrames::KeyFrame::operator==(const KeyFrame& rhs) const
{
	return _frame == rhs._frame;
}

bool ff::KeyFrames::KeyFrame::operator<(const KeyFrame& rhs) const
{
	return _frame < rhs._frame;
}

ff::KeyFrames::KeyFrames()
	: _start(0)
	, _length(0)
	, _method(MethodType::None)
{
}

ff::KeyFrames::KeyFrames(const KeyFrames& rhs)
	: _name(rhs._name)
	, _keys(rhs._keys)
	, _default(rhs._default)
	, _start(rhs._start)
	, _length(rhs._length)
	, _method(rhs._method)
{
}

ff::KeyFrames::KeyFrames(KeyFrames&& rhs)
	: _name(std::move(rhs._name))
	, _keys(std::move(rhs._keys))
	, _default(std::move(rhs._default))
	, _start(rhs._start)
	, _length(rhs._length)
	, _method(rhs._method)
{
}

ff::ValuePtr ff::KeyFrames::GetValue(float frame, const Dict* params)
{
	if (_keys.Size() && AdjustFrame(frame, _start, _length, _method))
	{
		size_t keyIndex;
		if (_keys.SortFind(*reinterpret_cast<const KeyFrame*>(&frame), &keyIndex) || keyIndex == 0)
		{
			return _keys[keyIndex]._value;
		}
		else if (keyIndex == _keys.Size())
		{
			return _keys.GetLast()._value;
		}
		else
		{
			const KeyFrame& prevKey = _keys[keyIndex - 1];
			const KeyFrame& nextKey = _keys[keyIndex];

			float time = (frame - prevKey._frame) / (nextKey._frame - prevKey._frame);
			return Interpolate(prevKey, nextKey, time, params);
		}
	}

	return _default && !_default->IsType<ff::NullValue>() ? _default : nullptr;
}

float ff::KeyFrames::GetStart() const
{
	return _start;
}

float ff::KeyFrames::GetLength() const
{
	return _length;
}

ff::StringRef ff::KeyFrames::GetName() const
{
	return _name;
}

ff::KeyFrames ff::KeyFrames::LoadFromSource(ff::StringRef name, const Dict& dict, ff::IResourceLoadListener* loadListener)
{
	KeyFrames frames;
	assertRetVal(frames.LoadFromSourceInternal(name, dict, loadListener), KeyFrames());
	return frames;
}

ff::KeyFrames ff::KeyFrames::LoadFromCache(const Dict& dict)
{
	KeyFrames frames;
	assertRetVal(frames.LoadFromCacheInternal(dict), KeyFrames());
	return frames;
}

ff::Dict ff::KeyFrames::SaveToCache() const
{
	ff::Dict dict;

	dict.Set<ff::StringValue>(::PROP_NAME, _name);
	dict.Set<ff::IntValue>(::PROP_METHOD, (int)_method);
	dict.Set<ff::FloatValue>(::PROP_START, _start);
	dict.Set<ff::FloatValue>(::PROP_LENGTH, _length);
	dict.SetValue(::PROP_DEFAULT, _default);

	ff::Vector<float> frames;
	ff::Vector<ff::ValuePtr> values;
	ff::Vector<ff::ValuePtr> tangents;

	frames.Reserve(_keys.Size());
	values.Reserve(_keys.Size());
	tangents.Reserve(_keys.Size());

	for (const KeyFrame& key : _keys)
	{
		frames.Push(key._frame);
		values.Push(key._value);
		tangents.Push(key._tangentValue);
	}

	dict.Set<ff::FloatVectorValue>(::PROP_FRAMES, std::move(frames));
	dict.Set<ff::ValueVectorValue>(::PROP_VALUES, std::move(values));
	dict.Set<ff::ValueVectorValue>(::PROP_TANGENTS, std::move(tangents));

	return dict;
}

bool ff::KeyFrames::LoadFromCacheInternal(const Dict& dict)
{
	_name = dict.Get<ff::StringValue>(::PROP_NAME);
	_method = (MethodType)dict.Get<ff::IntValue>(::PROP_METHOD);
	_start = dict.Get<ff::FloatValue>(::PROP_START);
	_length = dict.Get<ff::FloatValue>(::PROP_LENGTH);
	_default = dict.GetValue(::PROP_DEFAULT);

	ff::Vector<float> frames = dict.Get<ff::FloatVectorValue>(::PROP_FRAMES);
	ff::Vector<ff::ValuePtr> values = dict.Get<ff::ValueVectorValue>(::PROP_VALUES);
	ff::Vector<ff::ValuePtr> tangents = dict.Get<ff::ValueVectorValue>(::PROP_TANGENTS);
	assertRetVal(frames.Size() == values.Size() && frames.Size() == tangents.Size(), false);

	_keys.Reserve(frames.Size());
	for (size_t i = 0; i < frames.Size(); i++)
	{
		KeyFrame key;
		key._frame = frames[i];
		key._value = values[i];
		key._tangentValue = tangents[i];
		_keys.Push(std::move(key));
	}

	return true;
}

static ff::ValuePtr ConvertKeyValue(ff::ValuePtr value)
{
	// Check if it's an interpolatable value

	ff::ValuePtrT<ff::RectFloatValue> rectFloatValue = value;
	if (rectFloatValue)
	{
		return rectFloatValue;
	}

	ff::ValuePtrT<ff::PointFloatValue> pointFloatValue = value;
	if (pointFloatValue)
	{
		return pointFloatValue;
	}

	ff::ValuePtrT<ff::FloatValue> floatValue = value;
	if (floatValue)
	{
		return floatValue;
	}

	return value ? value : ff::Value::New<ff::NullValue>();
}

bool ff::KeyFrames::LoadFromSourceInternal(ff::StringRef name, const Dict& dict, ff::IResourceLoadListener* loadListener)
{
	float tension = dict.Get<ff::FloatValue>(::PROP_TENSION, 0.0f);
	tension = (1.0f - tension) / 2.0f;

	ff::Vector<ff::ValuePtr> values = dict.Get<ff::ValueVectorValue>(::PROP_VALUES);
	for (ff::ValuePtr value : values)
	{
		ff::Dict valueDict = value->GetValue<ff::DictValue>();
		ff::ValuePtrT<ff::FloatValue> frameValue = valueDict.GetValue(::PROP_FRAME);
		ff::ValuePtr valueValue = valueDict.GetValue(::PROP_VALUE);
		if (!frameValue || !valueValue)
		{
			if (loadListener)
			{
				loadListener->AddError(ff::String::from_static(L"Key frame missing frame or value"));
			}

			assertRetVal(false, false);
		}

		KeyFrame key;
		key._frame = frameValue.GetValue();
		key._value = ::ConvertKeyValue(valueValue);
		key._tangentValue = ff::Value::New<ff::NullValue>();

		size_t keyPos;
		if (_keys.SortFind(key, &keyPos))
		{
			if (loadListener)
			{
				loadListener->AddError(ff::String::format_new(L"Key frame not unique: %g", key._frame));
			}

			assertRetVal(false, false);
		}

		_keys.Insert(keyPos, key);
	}

	_name = name;
	_start = dict.Get<ff::FloatValue>(::PROP_START, _keys.Size() ? _keys[0]._frame : 0.0f);
	_length = dict.Get<ff::FloatValue>(::PROP_LENGTH, _keys.Size() ? _keys.GetLast()._frame : 0.0f);
	_default = ::ConvertKeyValue(dict.GetValue(::PROP_DEFAULT));
	_method = LoadMethod(dict, false);

	if (_length < 0.0f)
	{
		if (loadListener)
		{
			loadListener->AddError(ff::String::format_new(L"Invalid key frame length: %g", _length));
		}

		assertRetVal(false, false);
	}

	// Init tangents

	for (size_t i = 0; i < _keys.Size(); i++)
	{
		KeyFrame& keyBefore = _keys[i ? i - 1 : _keys.Size() - 1];
		KeyFrame& keyAfter = _keys[i + 1 < _keys.Size() ? i + 1 : 0];

		if (keyBefore._value->IsSameType(keyAfter._value))
		{
			if (keyBefore._value->IsType<ff::FloatValue>())
			{
				float v1 = keyBefore._value->GetValue<ff::FloatValue>();
				float v2 = keyAfter._value->GetValue<ff::FloatValue>();

				_keys[i]._tangentValue = ff::Value::New<ff::FloatValue>(tension * (v2 - v1));
			}
			else if (keyBefore._value->IsType<ff::PointFloatValue>())
			{
				ff::PointFloat v1 = keyBefore._value->GetValue<ff::PointFloatValue>();
				ff::PointFloat v2 = keyAfter._value->GetValue<ff::PointFloatValue>();

				ff::RectFloat output;
				DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & output,
					DirectX::XMVectorMultiply(
						DirectX::XMVectorReplicate(tension),
						DirectX::XMVectorSubtract(
							DirectX::XMLoadFloat2((DirectX::XMFLOAT2*) & v2),
							DirectX::XMLoadFloat2((DirectX::XMFLOAT2*) & v1))));

				_keys[i]._tangentValue = ff::Value::New<ff::PointFloatValue>(output.TopLeft());
			}
			else if (keyBefore._value->IsType<ff::RectFloatValue>())
			{
				ff::RectFloat v1 = keyBefore._value->GetValue<ff::RectFloatValue>();
				ff::RectFloat v2 = keyAfter._value->GetValue<ff::RectFloatValue>();

				ff::RectFloat output;
				DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & output,
					DirectX::XMVectorMultiply(
						DirectX::XMVectorReplicate(tension),
						DirectX::XMVectorSubtract(
							DirectX::XMLoadFloat4((DirectX::XMFLOAT4*) & v2),
							DirectX::XMLoadFloat4((DirectX::XMFLOAT4*) & v1))));

				_keys[i]._tangentValue = ff::Value::New<ff::RectFloatValue>(output);
			}
		}
	}

	return true;
}

ff::ValuePtr ff::KeyFrames::Interpolate(const KeyFrame& lhs, const KeyFrame& rhs, float time, const Dict* params)
{
	ValuePtr value = lhs._value;

	if (value->IsType<ff::StringValue>())
	{
		ff::String paramName = value->GetValue<ff::StringValue>();
		if (!std::wcsncmp(paramName.c_str(), L"param:", 6))
		{
			ff::StaticString paramName2(paramName.c_str() + 6, paramName.size() - 6);
			value = params->GetValue(paramName2);
		}
	}
	else if (lhs._value->IsSameType(rhs._value)) // Can only interplate the same types
	{
		// Spline interpolation
		if (lhs._tangentValue->IsSameType(lhs._value) && rhs._tangentValue->IsSameType(lhs._value))
		{
			if (lhs._value->IsType<ff::FloatValue>())
			{
				// Q(s) = (2s^3 - 3s^2 + 1)v1 + (-2s^3 + 3s^2)v2 + (s^3 - 2s^2 + s)t1 + (s^3 - s^2)t2
				float time2 = time * time;
				float time3 = time2 * time;
				float v1 = lhs._value->GetValue<ff::FloatValue>();
				float t1 = lhs._tangentValue->GetValue<ff::FloatValue>();
				float v2 = rhs._value->GetValue<ff::FloatValue>();
				float t2 = rhs._tangentValue->GetValue<ff::FloatValue>();

				float output =
					(2 * time3 - 3 * time2 + 1) * v1 +
					(-2 * time3 + 3 * time2) * v2 +
					(time3 - 2 * time2 + time) * t1 +
					(time3 - time2) * t2;

				value = ff::Value::New<ff::FloatValue>(output);
			}
			else if (lhs._value->IsType<ff::PointFloatValue>())
			{
				ff::PointFloat v1 = lhs._value->GetValue<ff::PointFloatValue>();
				ff::PointFloat t1 = lhs._tangentValue->GetValue<ff::PointFloatValue>();
				ff::PointFloat v2 = rhs._value->GetValue<ff::PointFloatValue>();
				ff::PointFloat t2 = rhs._tangentValue->GetValue<ff::PointFloatValue>();

				ff::RectFloat output;
				DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & output,
					DirectX::XMVectorHermite(
						DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & v1),
						DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & t1),
						DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & v2),
						DirectX::XMLoadFloat2((const DirectX::XMFLOAT2*) & t2),
						time));

				value = ff::Value::New<ff::PointFloatValue>(output.TopLeft());
			}
			else if (lhs._value->IsType<ff::RectFloatValue>())
			{
				ff::RectFloat v1 = lhs._value->GetValue<ff::RectFloatValue>();
				ff::RectFloat t1 = lhs._tangentValue->GetValue<ff::RectFloatValue>();
				ff::RectFloat v2 = rhs._value->GetValue<ff::RectFloatValue>();
				ff::RectFloat t2 = rhs._tangentValue->GetValue<ff::RectFloatValue>();

				ff::RectFloat output;
				DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & output,
					DirectX::XMVectorHermite(
						DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & v1),
						DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & t1),
						DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & v2),
						DirectX::XMLoadFloat4((const DirectX::XMFLOAT4*) & t2),
						time));

				value = ff::Value::New<ff::RectFloatValue>(output);
			}
		}
		else if (lhs._value->IsType<ff::FloatValue>())
		{
			float v1 = lhs._value->GetValue<ff::FloatValue>();
			float v2 = rhs._value->GetValue<ff::FloatValue>();
			float output = (v2 - v1) * time + v1;

			value = ff::Value::New<ff::FloatValue>(output);
		}
		else if (lhs._value->IsType<ff::PointFloatValue>())
		{
			ff::RectFloat v1(lhs._value->GetValue<ff::PointFloatValue>(), ff::PointFloat::Zeros());
			ff::RectFloat v2(rhs._value->GetValue<ff::PointFloatValue>(), ff::PointFloat::Zeros());

			ff::RectFloat output;
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & output,
				DirectX::XMVectorLerp(
					DirectX::XMLoadFloat4((DirectX::XMFLOAT4*) & v1),
					DirectX::XMLoadFloat4((DirectX::XMFLOAT4*) & v2),
					time));

			value = ff::Value::New<ff::PointFloatValue>(output.TopLeft());
		}
		else if (lhs._value->IsType<ff::RectFloatValue>())
		{
			ff::RectFloat v1 = lhs._value->GetValue<ff::RectFloatValue>();
			ff::RectFloat v2 = rhs._value->GetValue<ff::RectFloatValue>();

			ff::RectFloat output;
			DirectX::XMStoreFloat4((DirectX::XMFLOAT4*) & output,
				DirectX::XMVectorLerp(
					DirectX::XMLoadFloat4((DirectX::XMFLOAT4*) & v1),
					DirectX::XMLoadFloat4((DirectX::XMFLOAT4*) & v2),
					time));

			value = ff::Value::New<ff::RectFloatValue>(output);
		}
	}

	return value;
}

ff::KeyFrames::MethodType ff::KeyFrames::LoadMethod(const ff::Dict& dict, bool fromCache)
{
	MethodType method = MethodType::None;

	if (fromCache || dict.GetValue(::PROP_METHOD)->IsType<ff::IntValue>())
	{
		method = (MethodType)dict.Get<ff::IntValue>(::PROP_METHOD);
	}
	else
	{
		ff::String methodString = dict.Get<ff::StringValue>(::PROP_METHOD, ff::String::from_static(L"linear"));
		method = ff::SetFlags(method, methodString == L"spline" ? MethodType::InterpolateSpline : MethodType::InterpolateLinear);

		ff::ValuePtr loopValue = dict.GetValue(::PROP_LOOP);
		if (!loopValue || loopValue->IsType<ff::BoolValue>())
		{
			method = ff::SetFlags(method, loopValue->GetValue<ff::BoolValue>() ? MethodType::BoundsLoop : MethodType::BoundsNone);
		}
		else if (loopValue->IsType<ff::StringValue>() && loopValue->GetValue<ff::StringValue>() == L"clamp")
		{
			method = ff::SetFlags(method, MethodType::BoundsClamp);
		}
	}

	return method;
}

bool ff::KeyFrames::AdjustFrame(float& frame, float start, float length, ff::KeyFrames::MethodType method)
{
	if (frame < start)
	{
		switch (ff::GetFlags(method, MethodType::BoundsBits))
		{
		default:
			return false;

		case MethodType::BoundsClamp:
			frame = start;
			break;

		case MethodType::BoundsLoop:
			frame = (length != 0)  ? std::fmodf(frame - start, length) + start : start;
			break;
		}
	}
	else if (frame >= start + length)
	{
		switch (ff::GetFlags(method, MethodType::BoundsBits))
		{
		default:
			return false;

		case MethodType::BoundsClamp:
			frame = start + length;
			break;

		case MethodType::BoundsLoop:
			frame = (length != 0) ? std::fmodf(frame - start, length) + start : start;
			break;
		}
	}

	return true;
}

ff::CreateKeyFrames::CreateKeyFrames(ff::StringRef name, float start, float length, KeyFrames::MethodType method, ValuePtr defaultValue)
{
	_dict.Set<ff::StringValue>(::PROP_NAME, name);
	_dict.Set<ff::FloatValue>(::PROP_START, start);
	_dict.Set<ff::FloatValue>(::PROP_LENGTH, length);
	_dict.Set<ff::IntValue>(::PROP_METHOD, (int)method);
	_dict.SetValue(::PROP_DEFAULT, defaultValue);
}

ff::CreateKeyFrames::CreateKeyFrames(const CreateKeyFrames& rhs)
	: _dict(rhs._dict)
	, _values(rhs._values)
{
}

ff::CreateKeyFrames::CreateKeyFrames(CreateKeyFrames&& rhs)
	: _dict(std::move(rhs._dict))
	, _values(std::move(rhs._values))
{
}

void ff::CreateKeyFrames::AddFrame(float frame, ValuePtr value)
{
	ff::Dict dict;
	dict.Set<ff::FloatValue>(::PROP_FRAME, frame);
	dict.SetValue(::PROP_VALUE, value);

	_values.Push(ff::Value::New<ff::DictValue>(std::move(dict)));
}

ff::KeyFrames ff::CreateKeyFrames::Create() const
{
	ff::Dict dict = _dict;
	dict.Set<ff::ValueVectorValue>(::PROP_VALUES, ff::Vector<ff::ValuePtr>(_values));

	return KeyFrames::LoadFromSource(dict.Get<ff::StringValue>(::PROP_NAME), dict);
}
