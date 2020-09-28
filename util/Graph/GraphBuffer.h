#pragma once
#pragma once

#include "Graph/GraphDeviceChild.h"

namespace ff
{
	class IGraphBuffer11;

	enum class GraphBufferType
	{
		Unknown,
		Vertex,
		Index,
		Constant,
	};

	class __declspec(uuid("8bd61f6a-5b19-4adf-b423-459877a73d9a")) __declspec(novtable)
		IGraphBuffer : public IUnknown, public IGraphDeviceChild
	{
	public:
		virtual GraphBufferType GetType() const = 0;
		virtual size_t GetSize() const = 0;
		virtual bool IsWritable() const = 0;
		virtual void* Map(size_t size) = 0;
		virtual void Unmap() = 0;

		virtual IGraphBuffer11* AsGraphBuffer11() = 0;
	};

	class IGraphBuffer11
	{
	public:
		virtual ID3D11Buffer* GetBuffer() = 0;
	};
}
