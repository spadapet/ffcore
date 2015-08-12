#pragma once

namespace ff
{
	class Mutex
	{
	public:
		UTIL_API Mutex();
		UTIL_API ~Mutex();

		UTIL_API void Enter() const;
		UTIL_API bool TryEnter() const;
		UTIL_API void Leave() const;
		UTIL_API bool WaitForCondition(CONDITION_VARIABLE& condition) const;

	private:
		Mutex(const Mutex& rhs) = delete;
		const Mutex& operator=(const Mutex& rhs) = delete;

		mutable CRITICAL_SECTION mutex;
	};

	class Condition
	{
	public:
		UTIL_API Condition();
		UTIL_API operator CONDITION_VARIABLE& ();
		UTIL_API void WakeOne();
		UTIL_API void WakeAll();

	private:
		CONDITION_VARIABLE condition;
	};

	class LockMutex
	{
	public:
		UTIL_API LockMutex(const Mutex& mutex);
		UTIL_API LockMutex(LockMutex&& rhs);
		UTIL_API ~LockMutex();

		UTIL_API void Unlock();

	private:
		const Mutex* mutex;
	};
}
