#include "pch.h"
#include "Globals/ThreadGlobals.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"
#include "Windows/WinUtil.h"

static __declspec(thread) ff::ThreadGlobals* s_threadGlobals = nullptr;

ff::ThreadGlobals::ThreadGlobals()
	: _state(State::UNSTARTED)
	, _id(::GetCurrentThreadId())
{
	assert(s_threadGlobals == nullptr);
	s_threadGlobals = this;
}

ff::ThreadGlobals::~ThreadGlobals()
{
	assert(s_threadGlobals == this && IsShuttingDown());
	s_threadGlobals = nullptr;
}

ff::ThreadGlobals* ff::ThreadGlobals::Get()
{
	assert(s_threadGlobals);
	return s_threadGlobals;
}

bool ff::ThreadGlobals::Exists()
{
	return s_threadGlobals != nullptr;
}

void ff::ThreadGlobals::Startup()
{
	_state = State::STARTED;
}

void ff::ThreadGlobals::Shutdown()
{
	CallShutdownFunctions();

	if (_dispatch)
	{
		_dispatch->Destroy();
		_dispatch = nullptr;
	}
}

ff::IThreadDispatch* ff::ThreadGlobals::GetDispatch()
{
	if (!_dispatch)
	{
		ff::LockMutex lock(_mutex);
		if (!_dispatch)
		{
			_dispatch = ff::CreateCurrentThreadDispatch();
		}

	}
	return _dispatch;
}

unsigned int ff::ThreadGlobals::ThreadId() const
{
	return _id;
}

bool ff::ThreadGlobals::IsValid() const
{
	switch (_state)
	{
	case State::UNSTARTED:
	case State::FAILED:
		return false;

	default:
		return true;
	}
}

bool ff::ThreadGlobals::IsShuttingDown() const
{
	return _state == State::SHUT_DOWN;
}

void ff::ThreadGlobals::AtShutdown(std::function<void()> func)
{
	assert(!IsShuttingDown());

	ff::LockMutex lock(_mutex);
	_shutdownFunctions.Push(std::move(func));
}

void ff::ThreadGlobals::CallShutdownFunctions()
{
	_state = State::SHUT_DOWN;

	Vector<std::function<void()>> shutdownFunctions;
	{
		ff::LockMutex lock(_mutex);
		shutdownFunctions.Reserve(_shutdownFunctions.Size());

		while (!_shutdownFunctions.IsEmpty())
		{
			shutdownFunctions.Push(std::move(*_shutdownFunctions.GetLast()));
			_shutdownFunctions.DeleteLast();
		}
	}

	for (const auto& func : shutdownFunctions)
	{
		func();
	}
}

void ff::AtThreadShutdown(std::function<void()> func)
{
	assertRet(s_threadGlobals != nullptr);

	if (s_threadGlobals->IsShuttingDown())
	{
		assertSz(false, L"Why register a thread shutdown function during shutdown?");
		func();
	}
	else
	{
		s_threadGlobals->AtShutdown(func);
	}
}

#if !METRO_APP

bool ff::ThreadGlobals::AddWindowListener(HWND hwnd, ff::IWindowProcListener* listener)
{
	if (hwnd)
	{
		auto kv = _windowListeners.GetKey(hwnd);
		WindowListenerEntry* entry = kv ? &kv->GetEditableValue() : nullptr;

		if (!entry)
		{
			kv = _windowListeners.SetKey(hwnd, WindowListenerEntry());
			entry = &kv->GetEditableValue();
			entry->oldWndProc = SubclassWindow(hwnd, ff::ThreadGlobals::StaticWindowListenerProc);
			entry->callCount = 0;
			entry->destroyed = false;
		}

		if (std::find(entry->listeners.begin(), entry->listeners.end(), listener) == entry->listeners.end())
		{
			entry->listeners.InsertFirst(listener);
			return true;
		}
	}

	return false;
}

bool ff::ThreadGlobals::RemoveWindowListener(HWND hwnd, ff::IWindowProcListener* listener)
{
	auto kv = _windowListeners.GetKey(hwnd);
	WindowListenerEntry* entry = kv ? &kv->GetEditableValue() : nullptr;

	if (entry)
	{
		auto i = std::find(entry->listeners.begin(), entry->listeners.end(), listener);
		if (i != entry->listeners.end())
		{
			*i = nullptr;
			return true;
		}
	}

	return false;
}

LRESULT ff::ThreadGlobals::StaticWindowListenerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto kv = ff::ThreadGlobals::Get()->_windowListeners.GetKey(hwnd);
	WindowListenerEntry* entry = kv ? &kv->GetEditableValue() : nullptr;
	if (!entry)
	{
		assert(false);
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	bool sawNull = false;
	bool returnResult = false;
	LRESULT result = 0;
	WNDPROC oldWndProc = entry->oldWndProc;

	entry->callCount++;

	if (msg == WM_NCDESTROY)
	{
		entry->destroyed = true;
	}

	for (ff::IWindowProcListener* listener : entry->listeners)
	{
		if (listener)
		{
			if (listener->ListenWindowProc(hwnd, msg, wParam, lParam, result))
			{
				returnResult = true;
				break;
			}
		}
		else
		{
			sawNull = true;
		}
	}

	if (!--entry->callCount)
	{
		if (entry->destroyed)
		{
			ff::ThreadGlobals::Get()->_windowListeners.DeleteKey(*kv);
		}
		else if (sawNull)
		{
			for (ff::IWindowProcListener** listener = entry->listeners.GetFirst(); listener; )
			{
				if (!*listener)
				{
					listener = entry->listeners.Delete(*listener);
				}
				else
				{
					listener = entry->listeners.GetNext(*listener);
				}
			}
		}
	}

	return returnResult ? result : CallWindowProc(oldWndProc, hwnd, msg, wParam, lParam);
}

#endif
