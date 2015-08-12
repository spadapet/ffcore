#pragma once

namespace ff
{
	class ReaderWriterLock
	{
	public:
		UTIL_API ReaderWriterLock();

		UTIL_API void EnterRead() const;
		UTIL_API bool TryEnterRead() const;
		UTIL_API void EnterWrite() const;
		UTIL_API bool TryEnterWrite() const;
		UTIL_API void LeaveRead() const;
		UTIL_API void LeaveWrite() const;

	private:
		ReaderWriterLock(const ReaderWriterLock& rhs) = delete;
		const ReaderWriterLock& operator=(const ReaderWriterLock& rhs) = delete;

		mutable SRWLOCK lock;
	};

	class LockReader
	{
	public:
		UTIL_API LockReader(const ReaderWriterLock& lock);
		UTIL_API LockReader(LockReader&& rhs);
		UTIL_API ~LockReader();

		void Unlock();

	private:
		const ReaderWriterLock* lock;
	};

	class LockWriter
	{
	public:
		UTIL_API LockWriter(const ReaderWriterLock& lock);
		UTIL_API LockWriter(LockWriter&& rhs);
		UTIL_API ~LockWriter();

		void Unlock();

	private:
		const ReaderWriterLock* lock;
	};

	template<typename T>
	class LockObjectRead
	{
	public:
		LockObjectRead(T& obj, ReaderWriterLock& lock);
		LockObjectRead(LockObjectRead<T>&& rhs);

		T& Get() const;
		T* operator->() const;

	private:
		T& obj;
		LockReader lock;
	};

	template<typename T>
	class LockObjectWrite
	{
	public:
		LockObjectWrite(T& obj, ReaderWriterLock& lock);
		LockObjectWrite(LockObjectWrite<T>&& rhs);

		T& Get() const;
		T* operator->() const;

	private:
		T& obj;
		LockWriter lock;
	};
}

template<typename T>
ff::LockObjectRead<T>::LockObjectRead(T& obj, ReaderWriterLock& lock)
	: obj(obj)
	, lock(lock)
{
}

template<typename T>
ff::LockObjectRead<T>::LockObjectRead(LockObjectRead<T>&& rhs)
	: obj(rhs.obj)
	, lock(std::move(rhs.lock))
{
}

template<typename T>
T& ff::LockObjectRead<T>::Get() const
{
	return this->obj;
}

template<typename T>
T* ff::LockObjectRead<T>::operator->() const
{
	return &this->obj;
}

template<typename T>
ff::LockObjectWrite<T>::LockObjectWrite(T& obj, ReaderWriterLock& lock)
	: obj(obj)
	, lock(lock)
{
}

template<typename T>
ff::LockObjectWrite<T>::LockObjectWrite(LockObjectWrite<T>&& rhs)
	: obj(rhs.obj)
	, lock(std::move(rhs.lock))
{
}

template<typename T>
T& ff::LockObjectWrite<T>::Get() const
{
	return this->obj;
}

template<typename T>
T* ff::LockObjectWrite<T>::operator->() const
{
	return &this->obj;
}
