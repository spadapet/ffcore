#include "pch.h"
#include "Globals/Log.h"

#include <crtdbg.h>

static std::atomic_size_t s_totalBytes;
static std::atomic_size_t s_currentBytes;
static std::atomic_size_t s_maxBytes;
static std::atomic_size_t s_allocCount;

ff::AtScope::AtScope(AtScope&& rhs)
	: _closeFunc(std::move(rhs._closeFunc))
{
}

ff::AtScope::AtScope(std::function<void()>&& closeFunc)
	: _closeFunc(std::move(closeFunc))
{
}

ff::AtScope::AtScope(std::function<void()>&& openFunc, std::function<void()>&& closeFunc)
	: _closeFunc(closeFunc)
{
	openFunc();
}

ff::AtScope::~AtScope()
{
	_closeFunc();
}

void ff::AtScope::Close()
{
	_closeFunc();
	_closeFunc = []() {};
}

ff::MemoryStats ff::GetMemoryAllocationStats()
{
	ff::MemoryStats stats;
	stats.total = s_totalBytes;
	stats.current = s_currentBytes;
	stats.maximum = s_maxBytes;
	stats.count = s_allocCount;

	return stats;
}

#ifdef _DEBUG

static int CrtAllocHook(
	int allocType,
	void* userData,
	size_t size,
	int blockType,
	long requestNumber,
	const unsigned char* filename,
	int lineNumber)
{
	// don't call any CRT functions when blockType == _CRT_BLOCK

	switch (allocType)
	{
	case _HOOK_ALLOC:
	case _HOOK_REALLOC:
	{
		s_allocCount.fetch_add(1);
		s_totalBytes.fetch_add(size);
		size_t currentBytes = s_currentBytes.fetch_add(size) + size;

		for (size_t currentMaxBytes = s_maxBytes, nextMaxBytes = std::max(currentBytes, currentMaxBytes); nextMaxBytes > currentMaxBytes; )
		{
			if (s_maxBytes.compare_exchange_weak(currentMaxBytes, nextMaxBytes))
			{
				break;
			}
		}
	}
	break;

	case _HOOK_FREE:
	{
		size = _msize(userData);

		for (size_t currentBytes = s_currentBytes, nextCurrentBytes; ; )
		{
			nextCurrentBytes = (size <= currentBytes) ? currentBytes - size : 0;

			if (s_currentBytes.compare_exchange_weak(currentBytes, nextCurrentBytes))
			{
				break;
			}
		}
	}
	break;
	}

	return TRUE;
}

#endif // _DEBUG

namespace ff
{
	void HookCrtMemAlloc();
	void UnhookCrtMemAlloc();
}

void ff::HookCrtMemAlloc()
{
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
	_CrtSetAllocHook(CrtAllocHook);
}

void ff::UnhookCrtMemAlloc()
{
	_CrtSetAllocHook(nullptr);

	Log::DebugTraceF(L"[MEM ALLOC] Count=%lu, Total=%lu, Max=%lu\n", s_allocCount.load(), s_totalBytes.load(), s_maxBytes.load());
}
