#pragma once

namespace ff
{
	template<typename T, typename TBase, BOOL(*DestroyHandleFunc)(TBase)>
	class WinHandleBase
	{
		typedef WinHandleBase<T, TBase, DestroyHandleFunc> MyType;

	public:
		WinHandleBase()
			: _handle(nullptr)
		{
		}

		WinHandleBase(MyType&& rhs)
			: _handle(rhs._handle)
		{
			rhs._handle = nullptr;
		}

		WinHandleBase(T handle)
			: _handle(handle)
		{
		}

		virtual ~WinHandleBase()
		{
			Close();
		}

		MyType& operator=(T handle)
		{
			if (_handle != handle)
			{
				Close();
				_handle = handle;
			}

			return *this;
		}

		void Close()
		{
			if (_handle)
			{
				DestroyHandleFunc(_handle);
				_handle = nullptr;
			}
		}

		void Attach(T handle)
		{
			Close();
			_handle = handle;
		}

		T Detach()
		{
			T handle = _handle;
			_handle = nullptr;
			return handle;
		}

		operator T() const
		{
			return _handle;
		}

		T* operator&()
		{
			assert(!_handle);
			return &_handle;
		}

	private:
		WinHandleBase(const MyType& rhs);
		MyType& operator=(const MyType& rhs);

		T _handle;
	};

	UTIL_API BOOL CloseHandle(HANDLE handle);
	UTIL_API HANDLE DuplicateHandle(HANDLE handle);

	typedef WinHandleBase<HANDLE, HANDLE, ff::CloseHandle> WinHandle;
}
