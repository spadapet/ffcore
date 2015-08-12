#pragma once

#if !METRO_APP

namespace ff
{
	UTIL_API void HandleMessage(MSG& msg); // filters and dispatches messages
	UTIL_API bool HandleMessages(); // handles all queued messages, returns false on WM_QUIT
	UTIL_API bool WaitForMessage(DWORD timeout = INFINITE); // waits for a single message and returns (false on WM_QUIT)
	UTIL_API bool GotQuitMessage(); // was WM_QUIT ever received?
	UTIL_API void PumpMessagesUntilQuit();

	class UTIL_API Window
	{
	public:
		Window();
		Window(Window&& rhs);
		virtual ~Window();

		HWND Handle() const;

	protected:
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
		virtual LRESULT DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
		virtual void PostNcDestroy();

		HWND _hwnd;

	private:
		Window(const Window& rhs) = delete;
		Window& operator=(const Window& rhs) = delete;
	};

	class UTIL_API CustomWindow : public Window
	{
	public:
		CustomWindow();
		CustomWindow(CustomWindow&& rhs);
		virtual ~CustomWindow();

		HWND Detach();
		bool Attach(HWND hwnd);

		static bool CreateClass(StringRef name, DWORD nStyle, HINSTANCE hInstance, HCURSOR hCursor, HBRUSH hBrush, UINT menu, HICON hLargeIcon, HICON hSmallIcon);
		static StringRef GetMessageWindowClassName();

		bool Create(StringRef className, StringRef windowName, HWND hParent, DWORD nStyle, DWORD nExStyle, int x, int y, int cx, int cy, HINSTANCE hInstance, HMENU hMenu);
		bool CreateBlank(StringRef windowName, HWND hParent, DWORD nStyle, DWORD nExStyle = 0, int x = 0, int y = 0, int cx = 0, int cy = 0, HMENU hMenu = nullptr);
		bool CreateBlank(HWND hParent, DWORD nStyle, DWORD nExStyle = 0, int x = 0, int y = 0, int cx = 0, int cy = 0);
		bool CreateMessageWindow(StringRef className = GetEmptyString(), StringRef windowName = GetEmptyString());

	protected:
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
		virtual LRESULT DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	private:
		CustomWindow(const CustomWindow& rhs) = delete;
		CustomWindow& operator=(const CustomWindow& rhs) = delete;

		static LRESULT CALLBACK BaseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static bool IsCustomWindow(HWND hwnd);
	};

	class IWindowProcListener
	{
	public:
		// return true to skip calling the default WindowProc
		virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT& nResult) = 0;
	};

	class UTIL_API ListenedWindow : public CustomWindow
	{
	public:
		ListenedWindow();
		ListenedWindow(ListenedWindow&& rhs);
		virtual ~ListenedWindow();

		void SetListener(IWindowProcListener* pListener);

	protected:
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	private:
		ListenedWindow(const ListenedWindow& rhs) = delete;
		ListenedWindow& operator=(const ListenedWindow& rhs) = delete;

		IWindowProcListener* _listener;
	};
}

#endif // METRO_APP
