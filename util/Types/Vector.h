#pragma once

namespace ff
{
	namespace details
	{
		template<typename T, size_t StackSize, typename Enabled = void>
		class StackVectorBase;

		template<typename T, size_t StackSize>
		class StackVectorBase<T, StackSize, std::enable_if_t<StackSize != 0>>
		{
		protected:
			T* GetStackItems()
			{
				return (T*)this->stack;
			}

		private:
			alignas(T) std::byte stack[sizeof(T) * StackSize];
		};

		template<typename T, size_t StackSize>
		class StackVectorBase<T, StackSize, std::enable_if_t<StackSize == 0>>
		{
		protected:
			T* GetStackItems()
			{
				return nullptr;
			}
		};

		class VectorTypeHelper
		{
		protected:
			template<class T2, class... Args>
			static typename std::enable_if_t<!std::is_trivially_constructible<T2, Args...>::value> ConstructInPlace(T2* data, Args&&... args)
			{
				::new(data) T2(std::forward<Args>(args)...);
			}

			template<class T2, class... Args>
			static typename std::enable_if_t<std::is_trivially_constructible<T2, Args...>::value> ConstructInPlace(T2* data, Args&&... args)
			{
				// No constructor
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_trivially_copy_constructible<T2>::value> CopyConstructInPlace(T2* data, const T2& source)
			{
				ConstructInPlace(data, source);
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_trivially_copy_constructible<T2>::value> CopyConstructInPlace(T2* data, const T2& source)
			{
				std::memcpy(data, &source, sizeof(T2));
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_trivially_move_constructible<T2>::value> MoveConstructInPlace(T2* data, T2&& source)
			{
				ConstructInPlace(data, std::move(source));
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_trivially_move_constructible<T2>::value> MoveConstructInPlace(T2* data, T2&& source)
			{
				std::memcpy(data, &source, sizeof(T2));
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_destructible<T2>::value> DestructItem(T2* data)
			{
				data->~T2();
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_destructible<T2>::value> DestructItem(T2* data)
			{
				// Cannot destruct
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_trivially_copy_constructible<T2>::value> CopyConstructItems(T2* destData, const T2* sourceData, size_t count)
			{
				assert(destData + count <= sourceData || destData >= sourceData + count);
				noAssertRet(count && destData != sourceData);

				for (T2* endDestData = destData + count; destData != endDestData; destData++, sourceData++)
				{
					CopyConstructInPlace(destData, *sourceData);
				}
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_trivially_copy_constructible<T2>::value> CopyConstructItems(T2* destData, const T2* sourceData, size_t count)
			{
				if (count && destData != sourceData)
				{
					std::memcpy(destData, sourceData, count * sizeof(T2));
				}
			}

			template<bool MightOverlap, typename T2>
			static typename std::enable_if_t<!std::is_trivially_move_constructible<T2>::value> MoveConstructItems(T2* destData, T2* sourceData, size_t count)
			{
				noAssertRet(count);

				if (destData < sourceData)
				{
					for (T2* endDestData = destData + count; destData != endDestData; destData++, sourceData++)
					{
						MoveConstructInPlace(destData, std::move(*sourceData));
						DestructItem(sourceData);
					}
				}
				else if (destData > sourceData)
				{
					for (T2* endDestData = destData, *curDestData = destData + count, *curSourceData = sourceData + count;
						curDestData != endDestData; curDestData--, curSourceData--)
					{
						MoveConstructInPlace(curDestData - 1, std::move(curSourceData[-1]));
						DestructItem(curSourceData - 1);
					}
				}
			}

			template<bool MightOverlap, typename T2>
			static typename std::enable_if_t<std::is_trivially_move_constructible<T2>::value> MoveConstructItems(T2* destData, T2* sourceData, size_t count)
			{
				if (count)
				{
					if constexpr (MightOverlap)
					{
						std::memmove(destData, sourceData, count * sizeof(T2));
					}
					else
					{
						std::memcpy(destData, sourceData, count * sizeof(T2));
					}
				}
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_copy_assignable<T2>::value> AssignCopyItem(T2* dest, const T2& source)
			{
				*dest = source;
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_copy_assignable<T2>::value> AssignCopyItem(T2* dest, const T2& source)
			{
				DestructItem(dest);
				CopyConstructInPlace(dest, source);
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_move_assignable<T2>::value> AssignMoveItem(T2* dest, T2&& source)
			{
				*dest = std::move(source);
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_move_assignable<T2>::value> AssignMoveItem(T2* dest, T2&& source)
			{
				DestructItem(dest);
				MoveConstructInPlace(dest, std::move(source));
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_destructible<T2>::value> DestructItems(T2* data, size_t count)
			{
				for (T2* endData = data + count; data != endData; data++)
				{
					DestructItem(data);
				}
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_destructible<T2>::value> DestructItems(T2* data, size_t count)
			{
				// No destructor
			}

			template<typename T2>
			static typename std::enable_if_t<!std::is_trivially_default_constructible<T2>::value> DefaultConstructItems(T2* data, size_t count)
			{
				for (T2* endData = data + count; data != endData; data++)
				{
					ConstructInPlace(data);
				}
			}

			template<typename T2>
			static typename std::enable_if_t<std::is_trivially_default_constructible<T2>::value> DefaultConstructItems(T2* data, size_t count)
			{
				// No constructor
			}
		};
	}

	template<typename T, size_t StackSize = 0, typename Allocator = MemAllocator<T>>
	class Vector
		: private ff::details::StackVectorBase<T, StackSize>
		, private ff::details::VectorTypeHelper
	{
		typedef Vector<T, StackSize, Allocator> MyType;
		typedef PushCollector<T, MyType> MyCollector;

	public:
		Vector();
		Vector(const MyType& rhs);
		Vector(MyType&& rhs);
		~Vector();

		// Operators
		MyType& operator=(const MyType& rhs);
		MyType& operator=(MyType&& rhs);
		bool operator==(const MyType& rhs) const;
		bool operator!=(const MyType& rhs) const;

		// Info about the vector
		size_t Size() const;
		size_t ByteSize() const;
		size_t Allocated() const;
		size_t BytesAllocated() const;
		bool IsEmpty() const;

		// Data size operations
		void Clear();
		void Reduce();
		void ClearAndReduce();
		void Resize(size_t size);
		void Reserve(size_t size);

		// Data
		const T* Data() const;
		T* Data();
		const T* Data(size_t pos, size_t num = 0) const;
		T* Data(size_t pos, size_t num = 0);
		const T* ConstData() const;
		const T* ConstData(size_t pos, size_t num = 0) const;
		void SetStaticData(const T* data, size_t count);
		MyCollector GetCollector();

		const T& operator[](size_t pos) const;
		T& operator[](size_t pos);

		const T& GetAt(size_t pos, size_t num = 1) const;
		T& GetAt(size_t pos, size_t num = 1);
		const T& GetLast() const;
		T& GetLast();

		// Data setters
		void SetAt(size_t pos, T&& data);
		void SetAt(size_t pos, const T& data);
		void Push(T&& data);
		void Push(const T& data);
		void Push(const T* data, size_t count);
		T Pop();

		void Insert(size_t pos, T&& data);
		void Insert(size_t pos, const T& data);
		void Insert(size_t pos, const T* data, size_t count);
		void InsertDefault(size_t pos, size_t count = 1);
		void Delete(size_t pos, size_t count = 1);
		bool DeleteItem(const T& data);

		template<class... Args> void SetAtEmplace(size_t pos, Args&&... args);
		template<class... Args> void PushEmplace(Args&&... args);
		template<class... Args> void InsertEmplace(size_t pos, Args&&... args);

		// Searching
		size_t Find(const T& data, size_t start = 0) const;
		bool Contains(const T& data) const;

		size_t SortFind(const T& data, size_t start = 0) const;
		bool SortFind(const T& data, size_t* pos, size_t start = 0) const;
		template<typename LessCompare>
		size_t SortFindFunc(const T& data, LessCompare compare, size_t start = 0) const;
		template<typename LessCompare>
		bool SortFindFunc(const T& data, size_t* pos, LessCompare compare, size_t start = 0) const;

		// C++ iterators
	public:
		template<typename IT>
		class Iterator
		{
			typedef Iterator<IT> MyType;

		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = IT;
			using difference_type = std::ptrdiff_t;
			using pointer = IT*;
			using reference = IT&;

			Iterator(IT* cur)
				: _cur(cur)
			{
			}

			Iterator(const MyType& rhs)
				: _cur(rhs._cur)
			{
			}

			IT& operator*() const
			{
				return *this->_cur;
			}

			IT* operator->() const
			{
				return this->_cur;
			}

			MyType& operator++()
			{
				this->_cur++;
				return *this;
			}

			MyType operator++(int)
			{
				MyType pre = *this;
				this->_cur++;
				return pre;
			}

			MyType& operator--()
			{
				this->_cur--;
				return *this;
			}

			MyType operator--(int)
			{
				MyType pre = *this;
				this->_cur--;
				return pre;
			}

			bool operator==(const MyType& rhs) const
			{
				return this->_cur == rhs._cur;
			}

			bool operator!=(const MyType& rhs) const
			{
				return this->_cur != rhs._cur;
			}

			bool operator<(const MyType& rhs) const
			{
				return this->_cur < rhs._cur;
			}

			bool operator<=(const MyType& rhs) const
			{
				return this->_cur <= rhs._cur;
			}

			bool operator>(const MyType& rhs) const
			{
				return this->_cur > rhs._cur;
			}

			bool operator>=(const MyType& rhs) const
			{
				return this->_cur >= rhs._cur;
			}

			MyType operator+(ptrdiff_t count) const
			{
				MyType iter = *this;
				iter += count;
				return iter;
			}

			MyType operator-(ptrdiff_t count) const
			{
				MyType iter = *this;
				iter -= count;
				return iter;
			}

			ptrdiff_t operator-(const MyType& rhs) const
			{
				return this->_cur - rhs._cur;
			}

			MyType& operator+=(ptrdiff_t count)
			{
				this->_cur += count;
				return *this;
			}

			MyType& operator-=(ptrdiff_t count)
			{
				this->_cur -= count;
				return *this;
			}

			const IT& operator[](ptrdiff_t pos) const
			{
				return this->_cur[pos];
			}

			IT& operator[](ptrdiff_t pos)
			{
				return this->_cur[pos];
			}

		private:
			IT* _cur;
		};

		typedef Iterator<T> iterator;
		typedef Iterator<const T> const_iterator;
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

		iterator begin() { return iterator(this->_data); }
		iterator end() { return iterator(this->_data + this->_size); }
		const_iterator begin() const { return const_iterator(this->_data); }
		const_iterator end() const { return const_iterator(this->_data + this->_size); }
		const_iterator cbegin() const { return const_iterator(this->_data); }
		const_iterator cend() const { return const_iterator(this->_data + this->_size); }

		reverse_iterator rbegin() { return reverse_iterator(this->end()); }
		reverse_iterator rend() { return reverse_iterator(this->begin()); }
		const_reverse_iterator rbegin() const { return const_reverse_iterator(this->cend()); }
		const_reverse_iterator rend() const { return const_reverse_iterator(this->cbegin()); }
		const_reverse_iterator crbegin() const { return const_reverse_iterator(this->cend()); }
		const_reverse_iterator crend() const { return const_reverse_iterator(this->cbegin()); }

	private:
		void InsertEmpty(size_t pos, size_t count);
		bool IsStaticData() const;
		void EnsureEditable();
		void ReleaseData();

		T* _data;
		size_t _alloc;
		size_t _size;
	};
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::Vector()
	: _data(this->GetStackItems())
	, _alloc(StackSize)
	, _size(0)
{
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::Vector(const MyType& rhs)
	: _data(this->GetStackItems())
	, _alloc(StackSize)
	, _size(0)
{
	*this = rhs;
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::Vector(MyType&& rhs)
	: _data(this->GetStackItems())
	, _alloc(StackSize)
	, _size(0)
{
	*this = std::move(rhs);
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>::~Vector()
{
	this->DestructItems(this->_data, this->_size);
	this->ReleaseData();
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>& ff::Vector<T, StackSize, Allocator>::operator=(const MyType& rhs)
{
	if (this != &rhs)
	{
		this->Clear();
		this->Push(rhs._data, rhs._size);
	}

	return *this;
}

template<typename T, size_t StackSize, typename Allocator>
ff::Vector<T, StackSize, Allocator>& ff::Vector<T, StackSize, Allocator>::operator=(MyType&& rhs)
{
	if (this != &rhs)
	{
		this->ClearAndReduce();

		if (rhs._data == rhs.GetStackItems())
		{
			this->MoveConstructItems<false>(this->_data, rhs._data, rhs._size);
		}
		else
		{
			this->_data = rhs._data;
		}

		this->_alloc = rhs._alloc;
		this->_size = rhs._size;

		rhs._data = rhs.GetStackItems();
		rhs._alloc = StackSize;
		rhs._size = 0;
	}

	return *this;
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::operator==(const MyType& rhs) const
{
	if (this != &rhs)
	{
		if (this->_size != rhs._size)
		{
			return false;
		}

		for (size_t i = 0; i < this->_size; i++)
		{
			if (this->_data[i] != rhs._data[i])
			{
				return false;
			}
		}
	}

	return true;
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::operator!=(const MyType& rhs) const
{
	return !(*this == rhs);
}

template<typename T, size_t StackSize, typename Allocator>
const T* ff::Vector<T, StackSize, Allocator>::Data() const
{
	assert(this->_size);
	return this->_data;
}

template<typename T, size_t StackSize, typename Allocator>
const T* ff::Vector<T, StackSize, Allocator>::ConstData() const
{
	assert(this->_size);
	return this->_data;
}

template<typename T, size_t StackSize, typename Allocator>
T* ff::Vector<T, StackSize, Allocator>::Data()
{
	assert(this->_size);
	this->EnsureEditable();
	return this->_data;
}

template<typename T, size_t StackSize, typename Allocator>
const T* ff::Vector<T, StackSize, Allocator>::Data(size_t pos, size_t num) const
{
	assert(pos >= 0 && pos + num <= this->_size);
	return this->_data + pos;
}

template<typename T, size_t StackSize, typename Allocator>
T* ff::Vector<T, StackSize, Allocator>::Data(size_t pos, size_t num)
{
	assert(pos >= 0 && pos + num <= this->_size);
	this->EnsureEditable();
	return this->_data + pos;
}

template<typename T, size_t StackSize, typename Allocator>
const T* ff::Vector<T, StackSize, Allocator>::ConstData(size_t pos, size_t num) const
{
	assert(pos >= 0 && pos + num <= this->_size);
	return this->_data + pos;
}

template<typename T, size_t StackSize, typename Allocator>
ff::PushCollector<T, ff::Vector<T, StackSize, Allocator>> ff::Vector<T, StackSize, Allocator>::GetCollector()
{
	return ff::PushCollector<T, ff::Vector<T, StackSize, Allocator>>(*this);
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::Size() const
{
	return this->_size;
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::ByteSize() const
{
	return this->_size * sizeof(T);
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::IsEmpty() const
{
	return !this->_size;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Clear()
{
	this->Delete(0, this->_size);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::ClearAndReduce()
{
	this->Clear();
	this->Reduce();
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Resize(size_t size)
{
	if (size < this->_size)
	{
		this->Delete(size, this->_size - size);
	}
	else if (size > this->_size)
	{
		this->InsertDefault(this->_size, size - this->_size);
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Reserve(size_t size)
{
	if (size > this->_alloc)
	{
		size_t oldSize = this->_size;
		size_t alloc = std::max<size_t>(ff::NearestPowerOfTwo(this->_alloc), 8);

		if (size > alloc)
		{
			alloc = std::max<size_t>(alloc * 2, size);
		}

		T* data = Allocator::Malloc(alloc);
		this->MoveConstructItems<false>(data, this->_data, this->_size);
		this->ReleaseData();

		this->_data = data;
		this->_alloc = alloc;
		this->_size = oldSize;
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Reduce()
{
	if (this->_size < this->_alloc && this->_data != this->GetStackItems())
	{
		if (this->_size == 0)
		{
			this->ReleaseData();
		}
		else
		{
			T* data = (this->_size <= StackSize) ? this->GetStackItems() : Allocator::Malloc(this->_size);
			this->MoveConstructItems<false>(data, this->_data, this->_size);

			size_t size = this->_size;
			this->ReleaseData();

			this->_data = data;
			this->_size = size;
			this->_alloc = (this->_data == this->GetStackItems()) ? StackSize : this->_size;
		}
	}
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::Allocated() const
{
	return this->_alloc;
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::BytesAllocated() const
{
	return this->_alloc * sizeof(T);
}

template<typename T, size_t StackSize, typename Allocator>
const T& ff::Vector<T, StackSize, Allocator>::operator[](size_t pos) const
{
	assert(pos >= 0 && pos < this->_size);
	return this->_data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
T& ff::Vector<T, StackSize, Allocator>::operator[](size_t pos)
{
	assert(pos >= 0 && pos < this->_size);
	this->EnsureEditable();
	return this->_data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
const T& ff::Vector<T, StackSize, Allocator>::GetAt(size_t pos, size_t num) const
{
	assert(pos >= 0 && pos < this->_size && pos + num <= this->_size);
	return this->_data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
T& ff::Vector<T, StackSize, Allocator>::GetAt(size_t pos, size_t num)
{
	assert(pos >= 0 && pos < this->_size && pos + num <= this->_size);
	this->EnsureEditable();
	return this->_data[pos];
}

template<typename T, size_t StackSize, typename Allocator>
const T& ff::Vector<T, StackSize, Allocator>::GetLast() const
{
	assert(this->_size != 0);
	return this->_data[this->_size - 1];
}

template<typename T, size_t StackSize, typename Allocator>
T& ff::Vector<T, StackSize, Allocator>::GetLast()
{
	assert(this->_size);
	this->EnsureEditable();
	return this->_data[this->_size - 1];
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SetAt(size_t pos, T&& data)
{
	assert(pos >= 0 && pos < this->_size);

	this->EnsureEditable();
	this->AssignMoveItem(this->_data + pos, std::move(data));
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SetAt(size_t pos, const T& data)
{
	assert(pos >= 0 && pos < this->_size);

	if (this->_data + pos != &data)
	{
		this->EnsureEditable();
		this->AssignCopyItem(this->_data + pos, data);
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Push(T&& data)
{
	this->Insert(this->_size, std::move(data));
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Push(const T& data)
{
	this->Insert(this->_size, &data, 1);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Push(const T* data, size_t count)
{
	this->Insert(this->_size, data, count);
}

template<typename T, size_t StackSize, typename Allocator>
T ff::Vector<T, StackSize, Allocator>::Pop()
{
	assert(this->_size);

	this->EnsureEditable();
	T ret(std::move(this->_data[this->_size - 1]));
	this->Resize(this->_size - 1);
	return ret;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::SetStaticData(const T* data, size_t count)
{
	static_assert(std::is_trivially_copyable<T>::value, "Can only use static vector data with POD");

	assert(data);
	this->ClearAndReduce();
	this->_data = const_cast<T*>(data);
	this->_alloc = 0;
	this->_size = count;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Insert(size_t pos, T&& data)
{
	this->InsertEmpty(pos, 1);
	this->MoveConstructInPlace(this->_data + pos, std::move(data));
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Insert(size_t pos, const T& data)
{
	this->Insert(pos, &data, 1);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Insert(size_t pos, const T* data, size_t count)
{
	noAssertRet(count);
	assert(data && pos >= 0 && count >= 0 && pos <= this->_size);

	if (data >= this->_data + this->_alloc || data + count <= this->_data)
	{
		// Copying data from OUTSIDE the existing array
		this->InsertEmpty(pos, count);
		this->CopyConstructItems(this->_data + pos, data, count);
	}
	else
	{
		// Copying data from INSIDE the existing array, take the easy way
		// out and make another copy of the data first
		T* dataCopy = Allocator::Malloc(count);

		this->CopyConstructItems(dataCopy, data, count);
		this->Insert(pos, dataCopy, count);
		this->DestructItems(dataCopy, count);

		Allocator::Free(dataCopy);
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::InsertDefault(size_t pos, size_t count)
{
	this->InsertEmpty(pos, count);
	this->DefaultConstructItems(this->_data + pos, count);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::Delete(size_t pos, size_t count)
{
	assert(pos >= 0 && count >= 0 && pos + count <= this->_size);
	noAssertRet(count);

	this->EnsureEditable();
	this->DestructItems(this->_data + pos, count);

	size_t move = this->_size - pos - count;
	this->_size -= count;

	this->MoveConstructItems<true>(this->_data + pos, this->_data + pos + count, move);
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::DeleteItem(const T& data)
{
	size_t i = this->Find(data);
	if (i != ff::INVALID_SIZE)
	{
		this->Delete(i);
		return true;
	}

	return false;
}

template<typename T, size_t StackSize, typename Allocator>
template<class... Args>
void ff::Vector<T, StackSize, Allocator>::SetAtEmplace(size_t pos, Args&&... args)
{
	assert(pos >= 0 && pos < this->_size);

	this->EnsureEditable();
	this->DestructItem(this->_data + pos);
	this->ConstructInPlace(this->_data + pos, std::forward<Args>(args)...);
}

template<typename T, size_t StackSize, typename Allocator>
template<class... Args>
void ff::Vector<T, StackSize, Allocator>::PushEmplace(Args&&... args)
{
	this->InsertEmplace(this->_size, std::forward<Args>(args)...);
}

template<typename T, size_t StackSize, typename Allocator>
template<class... Args>
void ff::Vector<T, StackSize, Allocator>::InsertEmplace(size_t pos, Args&&... args)
{
	this->InsertEmpty(pos, 1);
	this->ConstructInPlace(this->_data + pos, std::forward<Args>(args)...);
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::Find(const T& data, size_t start) const
{
	if (&data >= this->_data + start && &data < this->_data + this->_size)
	{
		return &data - this->_data;
	}

	for (size_t i = start; i < this->_size; i++)
	{
		if (this->_data[i] == data)
		{
			return i;
		}
	}

	return ff::INVALID_SIZE;
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::Contains(const T& data) const
{
	return this->Find(data) != ff::INVALID_SIZE;
}

template<typename T, size_t StackSize, typename Allocator>
size_t ff::Vector<T, StackSize, Allocator>::SortFind(const T& data, size_t start) const
{
	return this->SortFindFunc(data, std:less<T>(), start);
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::SortFind(const T& data, size_t* pos, size_t start) const
{
	return this->SortFindFunc(data, pos, std::less<T>(), start);
}

template<typename T, size_t StackSize, typename Allocator>
template<typename LessCompare>
size_t ff::Vector<T, StackSize, Allocator>::SortFindFunc(const T& data, LessCompare compare, size_t start) const
{
	size_t pos;
	return this->SortFindFunc(data, &pos, compare, start) ? pos : ff::INVALID_SIZE;
}

template<typename T, size_t StackSize, typename Allocator>
template<typename LessCompare>
bool ff::Vector<T, StackSize, Allocator>::SortFindFunc(const T& data, size_t* pos, LessCompare compare, size_t start) const
{
	assert(start <= this->_size);

	auto found = std::lower_bound<const_iterator, const T&, LessCompare>(this->_data + start, this->end(), data, compare);
	if (pos != nullptr)
	{
		*pos = found - this->_data;
	}

	return found != this->end() && !compare(data, *found);
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::InsertEmpty(size_t pos, size_t count)
{
	assert(pos >= 0 && count >= 0 && pos <= this->_size);
	noAssertRet(count);

	this->Reserve(this->_size + count);

	if (pos < this->_size)
	{
		this->MoveConstructItems<true>(this->_data + pos + count, this->_data + pos, this->_size - pos);
	}

	this->_size += count;
}

template<typename T, size_t StackSize, typename Allocator>
bool ff::Vector<T, StackSize, Allocator>::IsStaticData() const
{
	return !this->_alloc;
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::EnsureEditable()
{
	if (this->IsStaticData())
	{
		this->Reserve(1);
	}
}

template<typename T, size_t StackSize, typename Allocator>
void ff::Vector<T, StackSize, Allocator>::ReleaseData()
{
	if (this->_data != this->GetStackItems())
	{
		if (!this->IsStaticData())
		{
			Allocator::Free(this->_data);
		}

		this->_data = this->GetStackItems();
	}

	this->_alloc = StackSize;
	this->_size = 0;
}
