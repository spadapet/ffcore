#pragma once

#include "Resource/Resources.h"
#include "Resource/ResourceValue.h"

namespace ff
{
	class XamlGlobalState;

	class XamlResourceCache : public ff::IResourceAccess
	{
	public:
		XamlResourceCache(XamlGlobalState* globals);

		void Advance();

		// IResourceAccess
		virtual SharedResourceValue GetResource(StringRef name) override;
		virtual Vector<String> GetResourceNames() const override;

	private:
		struct Entry
		{
			ff::SharedResourceValue _value;
			size_t _counter;
		};

		XamlGlobalState* _globals;
		ff::Map<ff::String, Entry> _cache;
	};
}
