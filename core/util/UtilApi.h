#pragma once

#if defined(UTIL_DLL)
#define UTIL_API __declspec(dllexport)
#define UTIL_IMPORT __declspec(dllimport)
#define UTIL_EXPORT __declspec(dllexport)
#else
#define UTIL_API __declspec(dllimport)
#define UTIL_IMPORT __declspec(dllimport)
# define UTIL_EXPORT __declspec(dllexport)
#endif

// External APIs

#include "Core/CrtPch.h"
#include "Windows/WindowsPch.h"
#include "Graph/DirectXPch.h"
#include <NoesisPCH.h>

// Internal basic stuff (order matters)

#include "Core/ConstantsPch.h"
#include "Core/AssertUtil.h"
#include "Core/FunctionsPch.h"
#include "Thread/Mutex.h"
#include "Thread/ReaderWriterLock.h"
#include "Types/Hash.h"
#include "Types/MemAlloc.h"
#include "Types/SmartPtr.h"
#include "COM/ComPch.h"

// Internal data type templates (order matters)

#include "Types/ItemCollector.h"
#include "Types/Vector.h"
#include "Types/PoolAllocator.h"
#include "Types/List.h"

#include "Types/Set.h"
#include "Types/KeyValue.h"
#include "Types/Map.h"

#include "Types/FixedInt.h"
#include "Types/Point.h"
#include "Types/Rect.h"

#include "String/String.h"

// There are tons of COM objects, so include the COM stuff

#include "COM/ComBase.h"
#include "COM/ComPtr.h"
#include "COM/ComUtil.h"
#include "Graph/DirectXUtil.h"
