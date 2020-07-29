#include "pch.h"
#include "Resource/Resources.h"
#include "UI/Internal/XamlResourceCache.h"
#include "UI/XamlGlobalState.h"

static const size_t MAX_COUNTER = 60;

ff::XamlResourceCache::XamlResourceCache(XamlGlobalState* globals)
	: _globals(globals)
{
}

void ff::XamlResourceCache::Advance()
{
	typedef ff::KeyValue<ff::String, Entry> KeyValue;
	ff::Vector<const KeyValue*, 32> entriesToDelete;

	for (const KeyValue& i : _cache)
	{
		Entry& entry = i.GetEditableValue();
		if (++entry._counter >= ::MAX_COUNTER)
		{
			entriesToDelete.Push(&i);
		}
	}

	for (const KeyValue* i : entriesToDelete)
	{
		_cache.DeleteKey(*i);
	}
}

ff::SharedResourceValue ff::XamlResourceCache::GetResource(ff::StringRef name)
{
	auto i = _cache.GetKey(name);
	if (!i)
	{
		ff::SharedResourceValue value = _globals->GetResourceAccess()->GetResource(name);
		i = _cache.SetKey(ff::String(name), Entry{ value, 0 });
	}

	Entry& entry = i->GetEditableValue();
	entry._counter = 0;
	return entry._value;
}

ff::Vector<ff::String> ff::XamlResourceCache::GetResourceNames() const
{
	return _globals->GetResourceAccess()->GetResourceNames();
}
