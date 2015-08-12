#pragma once

namespace ff
{
	class IThreadDispatch;
	class IWindowProcListener;

	class ThreadGlobals
	{
	public:
		UTIL_API ThreadGlobals();
		UTIL_API virtual ~ThreadGlobals();

		UTIL_API static ff::ThreadGlobals* Get();
		UTIL_API static bool Exists();

		UTIL_API virtual void Startup();
		UTIL_API virtual void Shutdown();

		UTIL_API unsigned int ThreadId() const;
		UTIL_API IThreadDispatch* GetDispatch();

		UTIL_API bool IsValid() const;
		UTIL_API bool IsShuttingDown() const;
		UTIL_API void AtShutdown(std::function<void()> func);

#if !METRO_APP
		UTIL_API bool AddWindowListener(HWND hwnd, IWindowProcListener* listener);
		UTIL_API bool RemoveWindowListener(HWND hwnd, IWindowProcListener* listener);
#endif

	protected:
		void CallShutdownFunctions();

		enum class State
		{
			UNSTARTED,
			STARTED,
			FAILED,
			SHUT_DOWN,
		} _state;

	private:
		ff::Mutex _mutex;
		unsigned int _id;
		ComPtr<IThreadDispatch> _dispatch;
		ff::List<std::function<void()>> _shutdownFunctions;

#if !METRO_APP
		struct WindowListenerEntry
		{
			WNDPROC oldWndProc;
			int callCount;
			bool destroyed;
			ff::List<IWindowProcListener*> listeners;
		};

		static LRESULT CALLBACK StaticWindowListenerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		ff::Map<HWND, WindowListenerEntry> _windowListeners;
#endif
	};

	UTIL_API void AtThreadShutdown(std::function<void()> func);
}
