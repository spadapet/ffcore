#pragma once

namespace ff
{
	template<typename Key, typename Value, typename Hash = Hasher<Key>>
	class Map
	{
		typedef KeyValue<Key, Value> KeyValueType;

		struct HashMapKey
		{
			static hash_t Hash(const KeyValueType& keyValue)
			{
				return Hash::Hash(keyValue.GetKey());
			}

			static bool Equals(const KeyValueType& lhs, const KeyValueType& rhs)
			{
				return lhs == rhs;
			}
		};

		typedef Set<KeyValueType, HashMapKey> SetType;

	public:
		Map();
		Map(const Map<Key, Value, Hash>& rhs);
		Map(Map<Key, Value, Hash>&& rhs);
		~Map();

		Map<Key, Value, Hash>& operator=(const Map<Key, Value, Hash>& rhs);

		size_t Size() const;
		bool IsEmpty() const;
		void Clear();
		void ClearAndReduce();
		void SetBucketCount(size_t count);

		const KeyValue<Key, Value>* SetKey(const Key& key, const Value& val); // doesn't allow duplicates
		const KeyValue<Key, Value>* SetKey(Key&& key, Value&& val); // doesn't allow duplicates
		const KeyValue<Key, Value>* InsertKey(const Key& key, const Value& val); // allows duplicates
		const KeyValue<Key, Value>* InsertKey(Key&& key, Value&& val); // allows duplicates
		bool UnsetKey(const Key& key);
		void DeleteKey(const KeyValue<Key, Value>& key);

		bool KeyExists(const Key& key) const;
		const KeyValue<Key, Value>* GetKey(const Key& key) const;
		const KeyValue<Key, Value>* GetNextDupeKey(const KeyValue<Key, Value>& key) const;

		typename SetType::const_iterator begin() const { return this->set.begin(); }
		typename SetType::const_iterator end() const { return this->set.end(); }
		typename SetType::const_iterator cbegin() const { return this->set.cbegin(); }
		typename SetType::const_iterator cend() const { return this->set.cend(); }

	private:
		SetType set;
	};
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::Map()
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::Map(const Map<Key, Value, Hash>& rhs)
	: set(rhs.set)
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::Map(Map<Key, Value, Hash>&& rhs)
	: set(std::move(rhs.set))
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>::~Map()
{
}

template<typename Key, typename Value, typename Hash>
ff::Map<Key, Value, Hash>& ff::Map<Key, Value, Hash>::operator=(const Map<Key, Value, Hash>& rhs)
{
	this->set = rhs.set;
	return *this;
}

template<typename Key, typename Value, typename Hash>
size_t ff::Map<Key, Value, Hash>::Size() const
{
	return this->set.Size();
}

template<typename Key, typename Value, typename Hash>
bool ff::Map<Key, Value, Hash>::IsEmpty() const
{
	return this->set.IsEmpty();
}

template<typename Key, typename Value, typename Hash>
void ff::Map<Key, Value, Hash>::Clear()
{
	this->set.Clear();
}

template<typename Key, typename Value, typename Hash>
void ff::Map<Key, Value, Hash>::ClearAndReduce()
{
	this->set.ClearAndReduce();
}

template<typename Key, typename Value, typename Hash>
void ff::Map<Key, Value, Hash>::SetBucketCount(size_t count)
{
	this->set.SetBucketCount(count);
}

template<typename Key, typename Value, typename Hash>
const ff::KeyValue<Key, Value>* ff::Map<Key, Value, Hash>::SetKey(const Key& key, const Value& val)
{
	return this->set.SetKey(key, val);
}

template<typename Key, typename Value, typename Hash>
const ff::KeyValue<Key, Value>* ff::Map<Key, Value, Hash>::SetKey(Key&& key, Value&& val)
{
	return this->set.SetKey(std::move(key), std::move(val));
}

template<typename Key, typename Value, typename Hash>
const ff::KeyValue<Key, Value>* ff::Map<Key, Value, Hash>::InsertKey(const Key& key, const Value& val)
{
	return this->set.InsertKey(key, val);
}

template<typename Key, typename Value, typename Hash>
const ff::KeyValue<Key, Value>* ff::Map<Key, Value, Hash>::InsertKey(Key&& key, Value&& val)
{
	return this->set.InsertKey(std::move(key), std::move(val));
}

template<typename Key, typename Value, typename Hash>
bool ff::Map<Key, Value, Hash>::UnsetKey(const Key& key)
{
	return this->set.UnsetKey(*(const KeyValueType*)&key);
}

template<typename Key, typename Value, typename Hash>
void ff::Map<Key, Value, Hash>::DeleteKey(const KeyValue<Key, Value>& key)
{
	return this->set.DeleteKey(key);
}

template<typename Key, typename Value, typename Hash>
bool ff::Map<Key, Value, Hash>::KeyExists(const Key& key) const
{
	return this->GetKey(key) != nullptr;
}

template<typename Key, typename Value, typename Hash>
const ff::KeyValue<Key, Value>* ff::Map<Key, Value, Hash>::GetKey(const Key& key) const
{
	return this->set.GetKey(*(const KeyValueType*)&key);
}

template<typename Key, typename Value, typename Hash>
const ff::KeyValue<Key, Value>* ff::Map<Key, Value, Hash>::GetNextDupeKey(const KeyValue<Key, Value>& key) const
{
	return this->set.GetNextDupeKey(key);
}
