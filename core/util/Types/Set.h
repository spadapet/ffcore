#pragma once

namespace ff
{
	template<typename Key, typename Hash = Hasher<Key>>
	class Set
	{
	public:
		Set();
		Set(const Set<Key, Hash>& rhs);
		Set(Set<Key, Hash>&& rhs);
		~Set();

		Set<Key, Hash>& operator=(const Set<Key, Hash>& rhs);

		size_t Size() const;
		bool IsEmpty() const;
		void Clear();
		void ClearAndReduce();
		bool SetBucketCount(size_t count);
		SetCollector<Key, Set<Key, Hash>> GetCollector();

		template<class... Args> const Key* SetKey(Args&&... args); // doesn't allow duplicates
		template<class... Args> const Key* InsertKey(Args&&... args); // allows duplicates
		bool UnsetKey(const Key& key); // deletes all matching keys
		void DeleteKey(const Key& key); // deletes one existing key

		bool KeyExists(const Key& key) const;
		const Key* GetKey(const Key& key) const;
		const Key* GetNextDupeKey(const Key& key) const;

	private:
		struct SetItem
		{
			Key key;
			ff::hash_t hash;
			SetItem* prevDupe;
			SetItem* nextDupe;
			SetItem* prevItem;
			SetItem* nextItem;

			template<class... Args> SetItem(Args&&... args) : key(std::forward<Args>(args)...) { }
			static SetItem* FromKey(const Key* key) { return reinterpret_cast<SetItem*>(const_cast<Key*>(key)); }
		};

		SetItem*& GetBucket(SetItem* item);
		SetItem*& GetBucket(hash_t hash);
		const SetItem* GetBucketConst(SetItem* item) const;
		const SetItem* GetBucketConst(hash_t hash) const;
		void EmptyBuckets();
		bool EnsureBucketsForInsert();

		template<class... Args> SetItem* NewItem(Args&&... args);
		void UnsetItem(SetItem* item);
		void DeleteItem(SetItem* item);
		const Key* InsertItem(SetItem* item, bool allowDupes, bool allowBucketResize);

		ff::Vector<SetItem*> buckets;
		ff::List<SetItem> items;
		size_t loadSize;

		// Imperfect C++ iterators
	public:
		template<typename IT>
		class Iterator
		{
			typedef Iterator<IT> MyType;
			typedef typename ff::List<SetItem>::const_iterator ListIterType;

		public:
			using iterator_category = std::input_iterator_tag;
			using value_type = IT;
			using difference_type = std::ptrdiff_t;
			using pointer = IT*;
			using reference = IT&;

			Iterator(ListIterType iter)
				: iter(iter)
			{
			}

			Iterator(const MyType& rhs)
				: iter(rhs.iter)
			{
			}

			const IT& operator*() const
			{
				return (*this->iter).key;
			}

			const IT* operator->() const
			{
				return &(*this->iter).key;
			}

			MyType& operator++()
			{
				this->iter++;
				return *this;
			}

			MyType operator++(int)
			{
				MyType pre = *this;
				this->iter++;
				return pre;
			}

			bool operator==(const MyType& rhs) const
			{
				return this->iter == rhs.this->iter;
			}

			bool operator!=(const MyType& rhs) const
			{
				return this->iter != rhs.iter;
			}

		private:
			ListIterType iter;
		};

		typedef Iterator<Key> const_iterator;

		const_iterator begin() const { return const_iterator(this->items.cbegin()); }
		const_iterator end() const { return const_iterator(this->items.cend()); }
		const_iterator cbegin() const { return const_iterator(this->items.cbegin()); }
		const_iterator cend() const { return const_iterator(this->items.cend()); }
	};
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::Set()
	: loadSize(0)
{
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::Set(const Set<Key, Hash>& rhs)
	: loadSize(0)
{
	*this = rhs;
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::Set(Set<Key, Hash>&& rhs)
	: buckets(std::move(rhs.buckets))
	, items(std::move(rhs.items))
	, loadSize(rhs.loadSize)
{
	rhs.loadSize = 0;
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>::~Set()
{
}

template<typename Key, typename Hash>
ff::Set<Key, Hash>& ff::Set<Key, Hash>::operator=(const Set<Key, Hash>& rhs)
{
	if (this != &rhs)
	{
		this->Clear();
		this->SetBucketCount(rhs.buckets.Size());

		for (SetItem& rhsItem : rhs.items)
		{
			SetItem* item = this->NewItem(rhsItem.key);
			item->hash = rhsItem.hash;
			this->InsertItem(item, true, false);
		}
	}

	return *this;
}

template<typename Key, typename Hash>
size_t ff::Set<Key, Hash>::Size() const
{
	return this->items.Size();
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::IsEmpty() const
{
	return !this->loadSize;
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::Clear()
{
	this->EmptyBuckets();
	this->items.Clear();
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::ClearAndReduce()
{
	this->loadSize = 0;
	this->buckets.ClearAndReduce();
	this->items.ClearAndReduce();
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::SetBucketCount(size_t count)
{
	count = ff::NearestPowerOfTwo(count);
	count = std::max<size_t>(count, 1 << 4);
	count = std::min<size_t>(count, 1 << 18);

	if (count != this->buckets.Size())
	{
		this->buckets.Resize(count);
		this->EmptyBuckets();

		for (SetItem& item : this->items)
		{
			this->InsertItem(&item, true, false);
		}

		return true;
	}

	return false;
}

template<typename Key, typename Hash>
ff::SetCollector<Key, ff::Set<Key, Hash>> ff::Set<Key, Hash>::GetCollector()
{
	return ff::SetCollector<Key, ff::Set<Key, Hash>>(*this);
}

template<typename Key, typename Hash>
template<class... Args>
const Key* ff::Set<Key, Hash>::SetKey(Args&&... args)
{
	SetItem* item = this->NewItem(std::forward<Args>(args)...);
	item->hash = Hash::Hash(item->key);
	return this->InsertItem(item, false, true);
}

template<typename Key, typename Hash>
template<class... Args>
const Key* ff::Set<Key, Hash>::InsertKey(Args&&... args)
{
	SetItem* item = this->NewItem(std::forward<Args>(args)...);
	item->hash = Hash::Hash(item->key);
	return this->InsertItem(item, true, true);
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::UnsetKey(const Key& key)
{
	SetItem* item = SetItem::FromKey(this->GetKey(key));
	if (!item)
	{
		return false;
	}

	this->UnsetItem(item);
	return true;
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::DeleteKey(const Key& key)
{
	SetItem* item = SetItem::FromKey(&key);
	this->DeleteItem(item);
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::KeyExists(const Key& key) const
{
	return this->GetKey(key) != nullptr;
}

template<typename Key, typename Hash>
const Key* ff::Set<Key, Hash>::GetKey(const Key& key) const
{
	if (!this->IsEmpty())
	{
		hash_t hash = Hash::Hash(key);
		const SetItem* bucket = this->GetBucketConst(hash);

		for (const SetItem* item = bucket; item; item = item->nextItem)
		{
			if (item->hash == hash && item->key == key)
			{
				return &item->key;
			}
		}
	}

	return nullptr;
}

template<typename Key, typename Hash>
const Key* ff::Set<Key, Hash>::GetNextDupeKey(const Key& key) const
{
	SetItem* item = SetItem::FromKey(&key);
	return item->nextDupe ? &item->nextDupe->key : nullptr;
}

template<typename Key, typename Hash>
typename ff::Set<Key, Hash>::SetItem*& ff::Set<Key, Hash>::GetBucket(SetItem* item)
{
	return this->GetBucket(item->hash);
}

template<typename Key, typename Hash>
typename ff::Set<Key, Hash>::SetItem*& ff::Set<Key, Hash>::GetBucket(hash_t hash)
{
	return this->buckets[hash & (this->buckets.Size() - 1)];
}

template<typename Key, typename Hash>
const typename ff::Set<Key, Hash>::SetItem* ff::Set<Key, Hash>::GetBucketConst(SetItem* item) const
{
	return this->GetBucketConst(item->hash);
}

template<typename Key, typename Hash>
const typename ff::Set<Key, Hash>::SetItem* ff::Set<Key, Hash>::GetBucketConst(hash_t hash) const
{
	return this->buckets[hash & (this->buckets.Size() - 1)];
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::EmptyBuckets()
{
	this->loadSize = 0;

	if (this->buckets.Size())
	{
		std::memset(this->buckets.Data(), 0, this->buckets.ByteSize());
	}
}

template<typename Key, typename Hash>
bool ff::Set<Key, Hash>::EnsureBucketsForInsert()
{
	const size_t doubleBucketSize = this->buckets.Size() * 2;

	if (this->loadSize >= doubleBucketSize)
	{
		return this->SetBucketCount(doubleBucketSize);
	}

	return false;
}

template<typename Key, typename Hash>
template<class... Args>
typename ff::Set<Key, Hash>::SetItem* ff::Set<Key, Hash>::NewItem(Args&&... args)
{
	return &this->items.Push(std::forward<Args>(args)...);
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::UnsetItem(SetItem* item)
{
	if (item->prevItem)
	{
		item->prevItem->nextItem = item->nextItem;
	}
	else
	{
		this->GetBucket(item) = item->nextItem;
	}

	if (item->nextItem)
	{
		item->nextItem->prevItem = item->prevItem;
	}

	for (SetItem* dupe; item; item = dupe)
	{
		dupe = item->nextDupe;
		this->items.Delete(*item);
	}

	assert(this->loadSize > 0);
	this->loadSize--;
}

template<typename Key, typename Hash>
void ff::Set<Key, Hash>::DeleteItem(SetItem* item)
{
	if (item->nextDupe)
	{
		item->nextDupe->prevDupe = item->prevDupe;
	}

	if (item->prevDupe)
	{
		item->prevDupe->nextDupe = item->nextDupe;
	}
	else
	{
		if (item->prevItem)
		{
			if (item->nextDupe)
			{
				item->prevItem->nextItem = item->nextDupe;
				item->nextDupe->prevItem = item->prevItem;
			}
			else
			{
				item->prevItem->nextItem = item->nextItem;
			}
		}
		else
		{
			this->GetBucket(item) = item->nextDupe ? item->nextDupe : item->nextItem;
		}

		if (item->nextItem)
		{
			if (item->nextDupe)
			{
				item->nextItem->prevItem = item->nextDupe;
				item->nextDupe->nextItem = item->nextItem;
			}
			else
			{
				item->nextItem->prevItem = item->prevItem;
			}
		}

		if (!item->nextDupe)
		{
			this->loadSize--;
		}
	}

	this->items.Delete(*item);
}

template<typename Key, typename Hash>
const Key* ff::Set<Key, Hash>::InsertItem(SetItem* item, bool allowDupes, bool allowBucketResize)
{
	SetItem* dupe = nullptr;
	SetItem** bucket = nullptr;

	if (!this->buckets.IsEmpty())
	{
		bucket = &this->GetBucket(item);

		for (dupe = *bucket; dupe; dupe = dupe->nextItem)
		{
			if (dupe->hash == item->hash && dupe->key == item->key)
			{
				break;
			}
		}
	}

	if (dupe && allowDupes)
	{
		item->nextItem = dupe->nextItem;
		item->prevItem = dupe->prevItem;
		item->nextDupe = dupe;
		item->prevDupe = nullptr;
		dupe->prevDupe = item;

		if (dupe->nextItem)
		{
			dupe->nextItem->prevItem = item;
			dupe->nextItem = nullptr;
		}

		if (dupe->prevItem)
		{
			dupe->prevItem->nextItem = item;
			dupe->prevItem = nullptr;
		}
		else
		{
			*bucket = item;
		}
	}
	else
	{
		if (dupe)
		{
			this->UnsetItem(dupe);
		}

		if (!allowBucketResize || !this->EnsureBucketsForInsert())
		{
			if (*bucket)
			{
				(*bucket)->prevItem = item;
			}

			item->nextItem = *bucket;
			item->prevItem = nullptr;
			item->nextDupe = nullptr;
			item->prevDupe = nullptr;
			*bucket = item;

			this->loadSize++;
		}
	}

	return &item->key;
}
