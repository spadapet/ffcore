#include "pch.h"

ff::ReaderWriterLock::ReaderWriterLock()
{
	::InitializeSRWLock(&this->lock);
}

void ff::ReaderWriterLock::EnterRead() const
{
	::AcquireSRWLockShared(&this->lock);
}

bool ff::ReaderWriterLock::TryEnterRead() const
{
	return ::TryAcquireSRWLockShared(&this->lock) != FALSE;
}

void ff::ReaderWriterLock::EnterWrite() const
{
	::AcquireSRWLockExclusive(&this->lock);
}

bool ff::ReaderWriterLock::TryEnterWrite() const
{
	return ::TryAcquireSRWLockExclusive(&this->lock) != FALSE;
}

void ff::ReaderWriterLock::LeaveRead() const
{
	::ReleaseSRWLockShared(&this->lock);
}

void ff::ReaderWriterLock::LeaveWrite() const
{
	::ReleaseSRWLockExclusive(&this->lock);
}

ff::LockReader::LockReader(const ReaderWriterLock& lock)
	: lock(&lock)
{
	this->lock->EnterRead();
}

ff::LockReader::LockReader(LockReader&& rhs)
	: lock(rhs.lock)
{
	rhs.lock = nullptr;
}

ff::LockReader::~LockReader()
{
	this->Unlock();
}

void ff::LockReader::Unlock()
{
	if (this->lock)
	{
		this->lock->LeaveRead();
	}
}

ff::LockWriter::LockWriter(const ReaderWriterLock& lock)
	: lock(&lock)
{
	this->lock->EnterWrite();
}

ff::LockWriter::LockWriter(LockWriter&& rhs)
	: lock(rhs.lock)
{
	rhs.lock = nullptr;
}

ff::LockWriter::~LockWriter()
{
	this->Unlock();
}

void ff::LockWriter::Unlock()
{
	if (this->lock)
	{
		this->lock->LeaveWrite();
	}
}
