#pragma once

#include "Types/Event.h"

namespace ff
{
	class AppGlobals;

	class __declspec(novtable) IDebugPages
	{
	public:
		virtual size_t GetDebugPageCount() const = 0;
		virtual void DebugUpdateStats(AppGlobals* globals, size_t page, bool updateFastNumbers) = 0;
		virtual String GetDebugName(size_t page) const = 0;
		virtual size_t GetDebugInfoCount(size_t page) const = 0;
		virtual String GetDebugInfo(size_t page, size_t index, DirectX::XMFLOAT4& color) const = 0;

		virtual size_t GetDebugToggleCount(size_t page) const = 0;
		virtual String GetDebugToggle(size_t page, size_t index, int& value) const = 0;
		virtual void DebugToggle(size_t page, size_t index) = 0;
	};
}
