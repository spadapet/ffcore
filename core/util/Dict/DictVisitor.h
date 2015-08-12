#pragma once

#include "Value/Value.h"

namespace ff
{
	class Dict;

	class DictVisitorBase
	{
	public:
		UTIL_API DictVisitorBase();
		UTIL_API virtual ~DictVisitorBase();

		UTIL_API ff::Dict VisitDict(const ff::Dict& dict, ff::Vector<ff::String>& errors);

	protected:
		UTIL_API virtual void AddError(ff::StringRef text);
		UTIL_API virtual bool IsAsyncAllowed(const ff::Dict& dict);

		UTIL_API ff::String GetPath() const;
		UTIL_API void PushPath(ff::StringRef name);
		UTIL_API void PopPath();
		UTIL_API bool IsRoot() const;

		UTIL_API virtual ff::ValuePtr TransformDict(const ff::Dict& dict);
		UTIL_API virtual ff::ValuePtr TransformDictAsync(const ff::Dict& dict);
		UTIL_API virtual ff::ValuePtr TransformVector(const ff::Vector<ff::ValuePtr>& values);
		UTIL_API virtual ff::ValuePtr TransformValue(ff::ValuePtr value);
		UTIL_API virtual ff::ValuePtr TransformRootValue(ff::ValuePtr value);

		UTIL_API virtual void OnAsyncThreadStarted(DWORD mainThreadId);
		UTIL_API virtual void OnAsyncThreadDone();

	private:
		ff::Mutex _mutex;
		ff::Vector<ff::String> _errors;
		ff::Map<DWORD, ff::Vector<ff::String>> _threadToPath;
	};
}
