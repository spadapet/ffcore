#pragma once
// Include this in your module's main CPP file only! (through MainUtilInclude.h)

static HINSTANCE s_delayLoadInstance = nullptr;

namespace ff
{
	static void SetDelayLoadInstance(HINSTANCE instance)
	{
		s_delayLoadInstance = instance;
	}

	static HINSTANCE GetDelayLoadInstance()
	{
		return s_delayLoadInstance;
	}
}

#if !METRO_APP

#include <delayimp.h>

namespace ff
{
	static void FixModulePath(PDelayLoadInfo loadInfo)
	{
		if (strchr(loadInfo->szDll, '\\'))
		{
			return;
		}

		static __declspec(thread) char s_path[MAX_PATH * 2];

		if (!::GetModuleFileNameA(s_delayLoadInstance, s_path, _countof(s_path)) || ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			return;
		}

		char* slash = strrchr(s_path, '\\');
		if (slash == nullptr)
		{
			return;
		}

		slash[1] = 0;

		if (strcat_s(s_path, loadInfo->szDll) != 0)
		{
			// path too long
			return;
		}

		WIN32_FIND_DATAA data;
		HANDLE findHandle = ::FindFirstFileA(s_path, &data);

		if (findHandle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		::FindClose(findHandle);

		loadInfo->szDll = s_path;
	}
}

FARPROC WINAPI DelayLoadHook(unsigned int notify, PDelayLoadInfo loadInfo)
{
	switch (notify)
	{
	case dliNotePreLoadLibrary:
		ff::FixModulePath(loadInfo);
		break;

	case dliFailGetProc:
	case dliFailLoadLib:
	case dliNoteEndProcessing:
	case dliNotePreGetProcAddress:
	case dliStartProcessing:
	default:
		break;
	}

	return nullptr;
}

const PfnDliHook __pfnDliNotifyHook2 = DelayLoadHook;

#endif // !METRO_APP
