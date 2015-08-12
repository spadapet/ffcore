#pragma once

namespace ff
{
	// Doubly-linked list
	//
	// Uses a pool allocator for memory locality of nodes and fast allocations.
	template<typename T>
	class List
	{
	public:
		List();
		List(const List<T>& rhs);
		List(List<T>&& rhs);
		~List();

		List<T>& operator=(const List<T>& rhs);

		template<class... Args> T& Push(Args&&... args);
		template<class... Args> T& InsertFirst(Args&&... args);
		template<class... Args> T& InsertAfter(const T* afterObj, Args&&... args);
		template<class... Args> T& InsertBefore(const T* beforeObj, Args&&... args);

		void MoveToFront(const T& obj);
		void MoveToBack(const T& obj);

		size_t Size() const;
		T PopFirst();
		T PopLast();
		T* Delete(const T& obj, bool after = true); // returns the object after or before the deleted object
		void DeleteFirst();
		void DeleteLast();
		void Clear();
		void ClearAndReduce();
		bool IsEmpty() const;

		T* GetFirst() const;
		T* GetLast() const;
		T* GetNext(const T& obj) const;
		T* GetPrev(const T& obj) const;
		T* Find(const T& obj) const;

		Vector<T> ToVector() const;
		Vector<T*> ToPointerVector();
		Vector<const T*> ToConstPointerVector() const;
		static List<T> FromVector(const Vector<T>& vec);
		PushCollector<T, List<T>> GetCollector();

	private:
		struct ListItem
		{
			template<class... Args>
			ListItem(Args&&... args)
				: val(std::forward<Args>(args)...)
				, next(nullptr)
				, prev(nullptr)
			{
			}

			T val;
			ListItem* next;
			ListItem* prev;
		};

		ListItem* ItemFromObject(const T* obj) const;
		T* ObjectFromItem(ListItem* item) const;
		T& InsertListItemAfter(ListItem* newItem, ListItem* afterItem);
		T& InsertListItemBefore(ListItem* newItem, ListItem* beforeItem);

		template<class... Args>
		ListItem* New(Args&&... args)
		{
			this->size++;
			return this->itemPool.New(std::forward<Args>(args)...);
		}

		ListItem* head;
		ListItem* tail;
		size_t size;
		PoolAllocator<ListItem, false> itemPool;

		// Imperfect C++ iterators
	public:
		template<typename IT, bool Reverse>
		class Iterator
		{
			typedef Iterator<IT, Reverse> MyType;

		public:
			using iterator_category = std::bidirectional_iterator_tag;
			using value_type = IT;
			using difference_type = std::ptrdiff_t;
			using pointer = IT*;
			using reference = IT&;

			Iterator(ListItem* item)
				: _item(item)
			{
			}

			Iterator(const MyType& rhs)
				: _item(rhs._item)
			{
			}

			IT& operator*() const
			{
				return this->_item->val;
			}

			IT* operator->() const
			{
				return &this->_item->val;
			}

			MyType& operator++()
			{
				this->_item = Reverse ? this->_item->prev : this->_item->next;
				return *this;
			}

			MyType operator++(int)
			{
				MyType pre = *this;
				this->_item = Reverse ? this->_item->prev : this->_item->next;
				return pre;
			}

			MyType& operator--()
			{
				this->_item = Reverse ? this->_item->next : this->_item->prev;
				return *this;
			}

			MyType operator--(int)
			{
				MyType pre = *this;
				this->_item = Reverse ? this->_item->next : this->_item->prev;
				return pre;
			}

			bool operator==(const MyType& rhs) const
			{
				return this->_item == rhs._item;
			}

			bool operator!=(const MyType& rhs) const
			{
				return this->_item != rhs._item;
			}

		private:
			ListItem* _item;
		};

		typedef Iterator<T, false> iterator;
		typedef Iterator<const T, false> const_iterator;
		typedef Iterator<T, true> reverse_iterator;
		typedef Iterator<const T, true> const_reverse_iterator;

		iterator begin() { return iterator(this->head); }
		iterator end() { return iterator(nullptr); }
		iterator begin() const { return iterator(this->head); }
		iterator end() const { return iterator(nullptr); }
		const_iterator cbegin() const { return const_iterator(this->head); }
		const_iterator cend() const { return const_iterator(nullptr); }

		reverse_iterator rbegin() { return reverse_iterator(this->tail); }
		reverse_iterator rend() { return reverse_iterator(nullptr); }
		reverse_iterator rbegin() const { return reverse_iterator(this->tail); }
		reverse_iterator rend() const { return reverse_iterator(nullptr); }
		const_reverse_iterator crbegin() const { return const_reverse_iterator(this->tail); }
		const_reverse_iterator crend() const { return const_reverse_iterator(nullptr); }
	};
}

template<typename T>
ff::List<T>::List()
	: head(nullptr)
	, tail(nullptr)
	, size(0)
{
}

template<typename T>
ff::List<T>::List(const List<T>& rhs)
	: head(nullptr)
	, tail(nullptr)
	, size(0)
{
	*this = rhs;
}

template<typename T>
ff::List<T>::List(List<T>&& rhs)
	: head(rhs.head)
	, tail(rhs.tail)
	, size(rhs.size)
	, itemPool(std::move(rhs.itemPool))
{
	rhs.head = nullptr;
	rhs.tail = nullptr;
	rhs.size = 0;
}

template<typename T>
ff::List<T>::~List()
{
	this->Clear();
}

template<typename T>
ff::List<T>& ff::List<T>::operator=(const List<T>& rhs)
{
	if (this != &rhs)
	{
		this->Clear();

		for (const T* obj = rhs.GetFirst(); obj; obj = rhs.GetNext(*obj))
		{
			this->Push(*obj);
		}
	}

	return *this;
}

template<typename T>
template<class... Args>
T& ff::List<T>::Push(Args&&... args)
{
	return this->InsertListItemAfter(this->New(std::forward<Args>(args)...), nullptr);
}

template<typename T>
template<class... Args>
T& ff::List<T>::InsertFirst(Args&&... args)
{
	return this->InsertListItemBefore(this->New(std::forward<Args>(args)...), nullptr);
}

template<typename T>
template<class... Args>
T& ff::List<T>::InsertAfter(const T* afterObj, Args&&... args)
{
	return this->InsertListItemAfter(this->New(std::forward<Args>(args)...), this->ItemFromObject(afterObj));
}

template<typename T>
template<class... Args>
T& ff::List<T>::InsertBefore(const T* beforeObj, Args&&... args)
{
	return this->InsertListItemBefore(this->New(std::forward<Args>(args)...), this->ItemFromObject(beforeObj));
}

template<typename T>
void ff::List<T>::MoveToFront(const T& obj)
{
	ListItem* item = this->ItemFromObject(&obj);

	if (item != this->head)
	{
		if (item == this->tail)
		{
			this->tail = item->prev;
		}

		if (item->prev != nullptr)
		{
			item->prev->next = item->next;
		}

		if (item->next != nullptr)
		{
			item->next->prev = item->prev;
		}

		item->next = head;
		item->prev = nullptr;

		this->head->prev = item;
		this->head = item;
	}
}

template<typename T>
void ff::List<T>::MoveToBack(const T& obj)
{
	ListItem* item = this->ItemFromObject(&obj);

	if (item != this->tail)
	{
		if (item == this->head)
		{
			this->head = item->next;
		}

		if (item->prev != nullptr)
		{
			item->prev->next = item->next;
		}

		if (item->next != nullptr)
		{
			item->next->prev = item->prev;
		}

		item->next = nullptr;
		item->prev = tail;

		this->tail->next = item;
		this->tail = item;
	}
}

template<typename T>
size_t ff::List<T>::Size() const
{
	return this->size;
}

template<typename T>
T ff::List<T>::PopFirst()
{
	assert(this->Size());
	T obj = *this->GetFirst();
	this->DeleteFirst();
	return obj;
}

template<typename T>
T ff::List<T>::PopLast()
{
	assert(this->Size());
	T obj = *this->GetLast();
	this->DeleteLast();
	return obj;
}

template<typename T>
T* ff::List<T>::Delete(const T& obj, bool after)
{
	ListItem* item = this->ItemFromObject(&obj);
	ListItem* nextItem = after ? item->next : item->prev;

	if (item == this->head)
	{
		this->head = item->next;
	}

	if (item == this->tail)
	{
		this->tail = item->prev;
	}

	if (item->prev != nullptr)
	{
		item->prev->next = item->next;
	}

	if (item->next != nullptr)
	{
		item->next->prev = item->prev;
	}

	this->size--;
	this->itemPool.Delete(item);

	return nextItem ? this->ObjectFromItem(nextItem) : nullptr;
}

template<typename T>
void ff::List<T>::DeleteFirst()
{
	if (this->GetFirst())
	{
		this->Delete(*this->GetFirst());
	}
}

template<typename T>
void ff::List<T>::DeleteLast()
{
	if (this->GetLast())
	{
		this->Delete(*this->GetLast());
	}
}

template<typename T>
void ff::List<T>::Clear()
{
	while (this->head != nullptr)
	{
		Delete(this->head->val);
	}
}

template<typename T>
void ff::List<T>::ClearAndReduce()
{
	this->Clear();
	this->itemPool.Reduce();
}

template<typename T>
bool ff::List<T>::IsEmpty() const
{
	return this->Size() == 0;
}

template<typename T>
T* ff::List<T>::GetFirst() const
{
	return this->head ? this->ObjectFromItem(this->head) : nullptr;
}

template<typename T>
T* ff::List<T>::GetLast() const
{
	return this->tail ? this->ObjectFromItem(this->tail) : nullptr;
}

template<typename T>
T* ff::List<T>::GetNext(const T& obj) const
{
	ListItem* item = this->ItemFromObject(&obj);
	return item && item->next ? this->ObjectFromItem(item->next) : nullptr;
}

template<typename T>
T* ff::List<T>::GetPrev(const T& obj) const
{
	ListItem* item = this->ItemFromObject(&obj);
	return item && item->prev ? this->ObjectFromItem(item->prev) : nullptr;
}

template<typename T>
T* ff::List<T>::Find(const T& obj) const
{
	for (ListItem* item = this->head; item; item = item->next)
	{
		if (item->val == obj)
		{
			return &item->val;
		}
	}

	return nullptr;
}

template<typename T>
ff::Vector<T> ff::List<T>::ToVector() const
{
	Vector<T> newVector;
	newVector.Reserve(this->Size());

	for (const T& iter : *this)
	{
		newVector.Push(iter);
	}

	return newVector;
}

template<typename T>
ff::Vector<T*> ff::List<T>::ToPointerVector()
{
	Vector<T*> newVector;
	newVector.Reserve(this->Size());

	for (T& iter : *this)
	{
		newVector.Push(&iter);
	}

	return newVector;
}

template<typename T>
ff::Vector<const T*> ff::List<T>::ToConstPointerVector() const
{
	Vector<const T*> newVector;
	newVector.Reserve(this->Size());

	for (const T& iter : *this)
	{
		newVector.Push(&iter);
	}

	return newVector;
}

template<typename T>
ff::List<T> ff::List<T>::FromVector(const Vector<T>& vec)
{
	List<T> newList;
	for (const T& iter : vec)
	{
		newList.Insert(iter);
	}

	return newList;
}

template<typename T>
ff::PushCollector<T, ff::List<T>> ff::List<T>::GetCollector()
{
	return PushCollector<T, List<T>>(*this);
}

template<typename T>
typename ff::List<T>::ListItem* ff::List<T>::ItemFromObject(const T* obj) const
{
	return const_cast<ListItem*>((const ListItem*)obj);
}

template<typename T>
T* ff::List<T>::ObjectFromItem(ListItem* item) const
{
	return (T*)item;
}

template<typename T>
T& ff::List<T>::InsertListItemAfter(ListItem* newItem, ListItem* afterItem)
{
	if (afterItem == nullptr)
	{
		afterItem = this->tail;
	}

	if (afterItem == nullptr)
	{
		this->head = newItem;
		this->tail = newItem;
		newItem->next = nullptr;
		newItem->prev = nullptr;
	}
	else if (afterItem == this->tail)
	{
		newItem->next = nullptr;
		newItem->prev = tail;
		this->tail->next = newItem;
		this->tail = newItem;
	}
	else
	{
		newItem->prev = afterItem;
		newItem->next = afterItem->next;
		afterItem->next->prev = newItem;
		afterItem->next = newItem;
	}

	return *this->ObjectFromItem(newItem);
}

template<typename T>
T& ff::List<T>::InsertListItemBefore(ListItem* newItem, ListItem* beforeItem)
{
	if (beforeItem == nullptr)
	{
		beforeItem = this->head;
	}

	if (beforeItem == nullptr)
	{
		this->head = newItem;
		this->tail = newItem;
		newItem->next = nullptr;
		newItem->prev = nullptr;
	}
	else if (beforeItem == head)
	{
		newItem->next = head;
		newItem->prev = nullptr;
		this->head->prev = newItem;
		this->head = newItem;
	}
	else
	{
		newItem->next = beforeItem;
		newItem->prev = beforeItem->prev;
		beforeItem->prev->next = newItem;
		beforeItem->prev = newItem;
	}

	return *this->ObjectFromItem(newItem);
}
