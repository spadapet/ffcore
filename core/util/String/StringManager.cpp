#include "pch.h"
#include "Globals/Log.h"
#include "String/StringManager.h"

ff::StringManager::StringManager()
{
}

ff::StringManager::~StringManager()
{
}

wchar_t* ff::StringManager::New(size_t count)
{
	wchar_t* str = nullptr;

	if (count > 256)
	{
		// Need room before the actual string to store a null IPoolAllocator
		BYTE* mem = new BYTE[sizeof(wchar_t) * count + sizeof(IPoolAllocator*)];
		*(IPoolAllocator**)mem = nullptr;
		str = (wchar_t*)(mem + sizeof(IPoolAllocator*));
	}
	else if (count > 128)
	{
		auto mem = _pool_256.New();
		mem->_pool = &_pool_256;
		str = mem->_str.data();
	}
	else if (count > 64)
	{
		auto mem = _pool_128.New();
		mem->_pool = &_pool_128;
		str = mem->_str.data();
	}
	else if (count > 32)
	{
		auto mem = _pool_64.New();
		mem->_pool = &_pool_64;
		str = mem->_str.data();
	}
	else
	{
		auto mem = _pool_32.New();
		mem->_pool = &_pool_32;
		str = mem->_str.data();
	}

	return str;
}

void ff::StringManager::Delete(wchar_t* str)
{
	assertRet(str != nullptr);

	IPoolAllocator** afterPool = (IPoolAllocator**)str;
	IPoolAllocator* pool = *(afterPool - 1);
	BYTE* structStart = (BYTE*)(afterPool - 1);

	if (pool == nullptr)
	{
		delete[] structStart;
	}
	else
	{
		pool->DeleteBytes(structStart);
	}
}

ff::StringBuffer* ff::StringManager::NewVector()
{
	return _vectorPool.New();
}

ff::StringBuffer* ff::StringManager::NewVector(const ff::StringBuffer& rhs)
{
	return _vectorPool.New(rhs);
}

void ff::StringManager::DeleteVector(ff::StringBuffer* str)
{
	return _vectorPool.Delete(str);
}
