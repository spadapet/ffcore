#pragma once

namespace ff
{
	struct IPoolAllocator
	{
		virtual ~IPoolAllocator() { }
		virtual void DeleteBytes(void* obj) = 0;
	};

	struct IBytePoolAllocator : public IPoolAllocator
	{
		virtual ~IBytePoolAllocator() override { }
		virtual void* NewBytes() = 0;
	};

	namespace details
	{
		template<size_t ByteSize, size_t ByteAlign>
		struct BytePool
		{
			union alignas(SLIST_ENTRY) alignas(ByteAlign) Node
			{
				SLIST_ENTRY entry;
				std::array<std::byte, ByteSize> item;
			};

			BytePool(size_t size, PSLIST_ENTRY& firstEntry, PSLIST_ENTRY& lastEntry)
				: size(std::max<size_t>(ff::NearestPowerOfTwo(size), 8))
				, nodes(std::make_unique<Node[]>(this->size))
			{
				for (size_t i = 0; i < this->size - 1; i++)
				{
					this->nodes[i].entry.Next = &this->nodes[i + 1].entry;
				}

				this->nodes[this->size - 1].entry.Next = nullptr;

				firstEntry = &this->nodes[0].entry;
				lastEntry = &this->nodes[this->size - 1].entry;
			}

			size_t Size() const
			{
				return this->size;
			}

		private:
			size_t size;
			std::unique_ptr<Node[]> nodes;
		};
	}

	template<size_t ByteSize, size_t ByteAlign, bool ThreadSafe = true>
	class BytePoolAllocator : public IBytePoolAllocator
	{
		typedef ff::details::BytePool<ByteSize, ByteAlign> Pool;

	public:
		BytePoolAllocator()
			: size(0)
		{
			::InitializeSListHead(&this->freeList);
		}

		BytePoolAllocator(BytePoolAllocator&& rhs)
			: pools(std::move(rhs.pools))
			, freeList(rhs.freeList)
			, size(rhs.size.load())
		{
			::InitializeSListHead(&rhs.freeList);
			rhs.size = 0;
		}

		virtual ~BytePoolAllocator() override
		{
			assert(!this->size);
			::InterlockedFlushSList(&this->freeList);
		}

		// IBytePoolAllocator
		virtual void* NewBytes() override
		{
			PSLIST_ENTRY freeEntry = nullptr;
			while (!freeEntry && !(freeEntry = ::InterlockedPopEntrySList(&this->freeList)))
			{
				ff::LockMutex lock(mutex);

				if (!(freeEntry = ::InterlockedPopEntrySList(&this->freeList)))
				{
					size_t lastSize = !this->pools.IsEmpty() ? this->pools.GetLast().Size() : 0;
					PSLIST_ENTRY firstEntry, lastEntry;
					this->pools.PushEmplace(lastSize * 2, firstEntry, lastEntry);

					::InterlockedPushListSListEx(&this->freeList, firstEntry, lastEntry, (ULONG)this->pools.GetLast().Size());
				}
			}

			this->size.fetch_add(1);
			return freeEntry;
		}

		virtual void DeleteBytes(void* obj) override
		{
			if (obj)
			{
				Pool::Node* node = (Pool::Node*)obj;
				::InterlockedPushEntrySList(&this->freeList, &node->entry);
				this->size.fetch_sub(1);
			}
		}

		void Reduce()
		{
			noAssertRet(!this->size);
			::InterlockedFlushSList(&this->freeList);
			this->pools.ClearAndReduce();
		}

	private:
		BytePoolAllocator(const BytePoolAllocator& rhs) = delete;
		BytePoolAllocator& operator=(const BytePoolAllocator& rhs) = delete;

		ff::Mutex mutex;
		ff::Vector<Pool> pools;
		SLIST_HEADER freeList;
		std::atomic_size_t size;
	};

	template<size_t ByteSize, size_t ByteAlign>
	class BytePoolAllocator<ByteSize, ByteAlign, false> : public IBytePoolAllocator
	{
		typedef ff::details::BytePool<ByteSize, ByteAlign> Pool;

	public:
		BytePoolAllocator()
			: firstFree(nullptr)
			, size(0)
		{
		}

		BytePoolAllocator(BytePoolAllocator&& rhs)
			: pools(std::move(rhs.pools))
			, firstFree(rhs.firstFree)
			, size(rhs.size)
		{
			rhs.firstFree = nullptr;
			rhs.size = 0;
		}

		virtual ~BytePoolAllocator() override
		{
			assert(!this->size);
		}

		// IBytePoolAllocator
		virtual void* NewBytes() override
		{
			if (!this->firstFree)
			{
				size_t lastSize = !this->pools.IsEmpty() ? this->pools.GetLast().Size() : 0;
				PSLIST_ENTRY lastEntry;
				this->pools.PushEmplace(lastSize * 2, this->firstFree, lastEntry);
			}

			PSLIST_ENTRY freeEntry = this->firstFree;
			this->firstFree = this->firstFree->Next;
			this->size++;

			return freeEntry;
		}

		virtual void DeleteBytes(void* obj) override
		{
			if (obj)
			{
				Pool::Node* node = (Pool::Node*)obj;
				node->entry.Next = this->firstFree;
				this->firstFree = &node->entry;
				this->size--;
			}
		}

		void Reduce()
		{
			noAssertRet(!this->size);
			this->firstFree = nullptr;
			this->pools.ClearAndReduce();
		}

	private:
		BytePoolAllocator(const BytePoolAllocator& rhs) = delete;
		BytePoolAllocator& operator=(const BytePoolAllocator& rhs) = delete;

		ff::Vector<Pool> pools;
		PSLIST_ENTRY firstFree;
		size_t size;
	};

	template<typename T, bool ThreadSafe = true>
	class PoolAllocator : public IPoolAllocator
	{
	public:
		PoolAllocator()
		{
		}

		PoolAllocator(PoolAllocator&& rhs)
			: byteAllocator(std::move(rhs.byteAllocator))
		{
		}

		virtual ~PoolAllocator() override
		{
		}

		template<class... Args> T* New(Args&&... args)
		{
			return ::new(this->byteAllocator.NewBytes()) T(std::forward<Args>(args)...);
		}

		void Delete(T* obj)
		{
			if (obj)
			{
				obj->~T();
				this->byteAllocator.DeleteBytes(obj);
			}
		}

		// IPoolAllocator
		virtual void DeleteBytes(void* obj) override
		{
			this->Delete((T*)obj);
		}

		void Reduce()
		{
			this->byteAllocator.Reduce();
		}

	private:
		PoolAllocator(const PoolAllocator& rhs) = delete;
		PoolAllocator& operator=(const PoolAllocator& rhs) = delete;

		BytePoolAllocator<sizeof(T), alignof(T), ThreadSafe> byteAllocator;
	};
}
