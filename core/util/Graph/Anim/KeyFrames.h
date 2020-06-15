#pragma once

#include "Value/Value.h"
#include "Value/ValueType.h"

namespace ff
{
	class Dict;
	class IResourceLoadListener;

	class KeyFrames
	{
	public:
		KeyFrames(const KeyFrames& rhs);
		KeyFrames(KeyFrames&& rhs);

		ValuePtr GetValue(float frame, const Dict* params = nullptr);
		float GetStart() const;
		float GetLength() const;
		ff::StringRef GetName() const;

		static KeyFrames LoadFromSource(ff::StringRef name, const Dict& dict, IResourceLoadListener* loadListener = nullptr);
		static KeyFrames LoadFromCache(const Dict& dict);
		Dict SaveToCache() const;

		enum class MethodType
		{
			None = 0x0,

			BoundsNone = 0x01,
			BoundsLoop = 0x02,
			BoundsClamp = 0x04,
			BoundsBits = 0x0F,

			InterpolateLinear = 0x10,
			InterpolateSpline = 0x20,
			InterpolateBits = 0xF0,
		};

		static MethodType LoadMethod(const ff::Dict& dict, bool fromCache);
		static bool AdjustFrame(float& frame, float start, float length, MethodType method);

	private:
		struct KeyFrame
		{
			bool operator==(const KeyFrame& rhs) const;
			bool operator<(const KeyFrame& rhs) const;

			float _frame;
			ValuePtr _value;
			ValuePtr _tangentValue;
		};

		KeyFrames();
		bool LoadFromCacheInternal(const Dict& dict);
		bool LoadFromSourceInternal(ff::StringRef name, const Dict& dict, IResourceLoadListener* loadListener);
		ValuePtr Interpolate(const KeyFrame& lhs, const KeyFrame& rhs, float time, const Dict* params);

		ff::String _name;
		ff::Vector<KeyFrame> _keys;
		ValuePtr _default;
		float _start;
		float _length;
		MethodType _method;
	};
}
