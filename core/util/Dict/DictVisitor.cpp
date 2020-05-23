#include "pch.h"
#include "Dict/Dict.h"
#include "Dict/DictVisitor.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadPool.h"
#include "Thread/ThreadUtil.h"
#include "Value/Values.h"
#include "Windows/Handles.h"

ff::DictVisitorBase::DictVisitorBase()
{
}

ff::DictVisitorBase::~DictVisitorBase()
{
}

ff::Dict ff::DictVisitorBase::VisitDict(const ff::Dict& dict, ff::Vector<ff::String>& errors)
{
	ff::ValuePtr transformedDictValue = TransformDict(dict);

	if (_errors.Size())
	{
		errors.Push(_errors.ConstData(), _errors.Size());
		_errors.Clear();
	}

	ff::ValuePtr outputDictValue = transformedDictValue->Convert<ff::DictValue>();
	return outputDictValue->GetValue<ff::DictValue>();
}

void ff::DictVisitorBase::AddError(ff::StringRef text)
{
	ff::LockMutex lock(_mutex);
	_errors.Push(ff::String::format_new(L"%d> %s : %s", ::GetCurrentThreadId(), GetPath().c_str(), text.c_str()));
}

bool ff::DictVisitorBase::IsAsyncAllowed(const ff::Dict& dict)
{
	return false;
}

ff::String ff::DictVisitorBase::GetPath() const
{
	ff::LockMutex lock(_mutex);
	auto iter = _threadToPath.GetKey(::GetCurrentThreadId());
	noAssertRetVal(iter, ff::GetEmptyString());
	const ff::Vector<ff::String>& paths = iter->GetValue();

	size_t space = 0;

	for (ff::StringRef part : paths)
	{
		space += part.size() + 1;
	}

	ff::String path;
	path.reserve(space);

	for (ff::StringRef part : paths)
	{
		if (path.size() && part.find('[') != 0)
		{
			path += L"/";
		}

		path += part;
	}

	return path;
}

void ff::DictVisitorBase::PushPath(ff::StringRef name)
{
	ff::LockMutex lock(_mutex);
	auto iter = _threadToPath.GetKey(::GetCurrentThreadId());
	if (!iter)
	{
		iter = _threadToPath.SetKey(::GetCurrentThreadId(), ff::Vector<ff::String>());
	}

	ff::Vector<ff::String>& paths = iter->GetEditableValue();
	paths.Push(name);
}

void ff::DictVisitorBase::PopPath()
{
	ff::LockMutex lock(_mutex);
	auto iter = _threadToPath.GetKey(::GetCurrentThreadId());
	assertRet(iter);

	ff::Vector<ff::String>& paths = iter->GetEditableValue();
	paths.Pop();

	if (paths.IsEmpty())
	{
		_threadToPath.DeleteKey(*iter);
	}
}

bool ff::DictVisitorBase::IsRoot() const
{
	ff::LockMutex lock(_mutex);
	return _threadToPath.GetKey(::GetCurrentThreadId()) == nullptr;
}

ff::ValuePtr ff::DictVisitorBase::TransformRootValue(ff::ValuePtr value)
{
	return TransformValue(value);
}

void ff::DictVisitorBase::OnAsyncThreadStarted(DWORD mainThreadId)
{
	ff::LockMutex lock(_mutex);
	auto iter = _threadToPath.GetKey(mainThreadId);
	noAssertRet(iter);

	ff::Vector<ff::String> paths = iter->GetValue();
	_threadToPath.SetKey(::GetCurrentThreadId(), std::move(paths));
}

void ff::DictVisitorBase::OnAsyncThreadDone()
{
	ff::LockMutex lock(_mutex);
	_threadToPath.UnsetKey(::GetCurrentThreadId());
}

ff::ValuePtr ff::DictVisitorBase::TransformDict(const ff::Dict& dict)
{
	if (IsAsyncAllowed(dict))
	{
		return TransformDictAsync(dict);
	}

	bool root = IsRoot();
	ff::Dict outputDict(dict);

	for (ff::StringRef name : dict.GetAllNames())
	{
		PushPath(name);

		ff::ValuePtr oldValue = dict.GetValue(name);
		ff::ValuePtr newValue = root
			? TransformRootValue(oldValue)
			: TransformValue(oldValue);
		outputDict.SetValue(name, newValue);

		PopPath();
	}

	return ff::Value::New<ff::DictValue>(std::move(outputDict));
}

ff::ValuePtr ff::DictVisitorBase::TransformDictAsync(const ff::Dict& dict)
{
	bool root = IsRoot();
	ff::Vector<ff::String> names = dict.GetAllNames();

	ff::Vector<ff::ValuePtr> values;
	values.Resize(names.Size());

	ff::Vector<ff::WinHandle> valueEvents;
	valueEvents.Resize(names.Size());

	DWORD mainThreadId = ::GetCurrentThreadId();

	for (size_t i = 0; i < names.Size(); i++)
	{
		values[i] = dict.GetValue(names[i]);
		valueEvents[i] = ff::CreateEvent();

		ff::GetThreadPool()->AddTask([this, root, mainThreadId, i, &names, &values, &valueEvents]()
			{
				OnAsyncThreadStarted(mainThreadId);
				PushPath(names[i]);

				values[i] = root
					? TransformRootValue(values[i])
					: TransformValue(values[i]);

				PopPath();
				OnAsyncThreadDone();

				::SetEvent(valueEvents[i]);
			});
	}

	ff::Vector<HANDLE, MAXIMUM_WAIT_OBJECTS> handles;
	handles.Reserve(valueEvents.Size());

	for (const ff::WinHandle& handle : valueEvents)
	{
		handles.Push(handle);
	}

	while (handles.Size())
	{
		DWORD handleCount = std::min<DWORD>(MAXIMUM_WAIT_OBJECTS, (DWORD)handles.Size());
		DWORD waitResult = ::WaitForMultipleObjects(handleCount, handles.ConstData(), TRUE, INFINITE);
		assert(waitResult < handleCount);

		handles.Delete(0, handleCount);
	}

	ff::Dict outputDict;
	outputDict.Reserve(names.Size());

	for (size_t i = 0; i < names.Size(); i++)
	{
		outputDict.SetValue(names[i], values[i]);
	}

	return ff::Value::New<ff::DictValue>(std::move(outputDict));
}

ff::ValuePtr ff::DictVisitorBase::TransformVector(const ff::Vector<ff::ValuePtr>& values)
{
	ff::Vector<ff::ValuePtr> outputValues;
	outputValues.Reserve(values.Size());

	for (ff::ValuePtr value : values)
	{
		PushPath(ff::String::format_new(L"[%lu]", outputValues.Size()));

		ff::ValuePtr newValue = TransformValue(value);
		if (newValue)
		{
			outputValues.Push(newValue);
		}

		PopPath();
	}

	return ff::Value::New<ff::ValueVectorValue>(std::move(outputValues));
}

ff::ValuePtr ff::DictVisitorBase::TransformValue(ff::ValuePtr value)
{
	ff::ValuePtr outputValue = value;

	if (value->IsType<ff::DictValue>())
	{
		outputValue = TransformDict(value->GetValue<ff::DictValue>());
	}
	else if (value->IsType<ff::SavedDictValue>())
	{
		ff::ValuePtrT<ff::DictValue> dictValue = value;
		if (dictValue)
		{
			outputValue = TransformDict(dictValue.GetValue());
		}
	}
	else if (value->IsType<ff::ValueVectorValue>())
	{
		outputValue = TransformVector(value->GetValue<ff::ValueVectorValue>());
	}

	return outputValue;
}
