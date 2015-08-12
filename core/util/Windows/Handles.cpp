#include "pch.h"
#include "Windows/Handles.h"

HANDLE ff::DuplicateHandle(HANDLE handle)
{
	HANDLE newHandle = nullptr;
	assertRetVal(::DuplicateHandle(::GetCurrentProcess(), handle, ::GetCurrentProcess(), &newHandle, 0, FALSE, DUPLICATE_SAME_ACCESS), nullptr);
	return newHandle;
}

BOOL ff::CloseHandle(HANDLE handle)
{
	return ::CloseHandle(handle);
}
