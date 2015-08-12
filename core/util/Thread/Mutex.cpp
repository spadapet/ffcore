#include "pch.h"

ff::Mutex::Mutex()
{
	::InitializeCriticalSectionEx(&this->mutex, 3000, 0);
}

ff::Mutex::~Mutex()
{
	::DeleteCriticalSection(&this->mutex);
}

void ff::Mutex::Enter() const
{
	::EnterCriticalSection(&this->mutex);
}

bool ff::Mutex::TryEnter() const
{
	return ::TryEnterCriticalSection(&this->mutex) != FALSE;
}

void ff::Mutex::Leave() const
{
	::LeaveCriticalSection(&this->mutex);
}

bool ff::Mutex::WaitForCondition(CONDITION_VARIABLE& condition) const
{
	return ::SleepConditionVariableCS(&condition, &this->mutex, INFINITE) != FALSE;
}

ff::Condition::Condition()
{
	::InitializeConditionVariable(&this->condition);
}

ff::Condition::operator CONDITION_VARIABLE& ()
{
	return this->condition;
}

void ff::Condition::WakeOne()
{
	::WakeConditionVariable(&this->condition);
}

void ff::Condition::WakeAll()
{
	::WakeAllConditionVariable(&this->condition);
}

ff::LockMutex::LockMutex(const Mutex& mutex)
	: mutex(&mutex)
{
	this->mutex->Enter();
}

ff::LockMutex::LockMutex(LockMutex&& rhs)
	: mutex(rhs.mutex)
{
	rhs.mutex = nullptr;
}

ff::LockMutex::~LockMutex()
{
	this->Unlock();
}

void ff::LockMutex::Unlock()
{
	if (this->mutex != nullptr)
	{
		this->mutex->Leave();
		this->mutex = nullptr;
	}
}
