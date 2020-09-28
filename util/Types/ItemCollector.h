#pragma once

namespace ff
{
	template<typename T>
	class ItemCollector
	{
	public:
		virtual void Push(T&& data) = 0;
		virtual void Push(const T& data) = 0;
	};

	template<typename T, typename VT>
	class PushCollector : public ItemCollector<T>
	{
	public:
		PushCollector(VT& vec);

		virtual void Push(T&& data) override;
		virtual void Push(const T& data) override;

	private:
		VT& _vec;
	};

	template<typename T, typename ST>
	class SetCollector : public ItemCollector<T>
	{
	public:
		SetCollector(ST& set);

		virtual void Push(T&& data) override;
		virtual void Push(const T& data) override;

	private:
		ST& _set;
	};
}

template<typename T, typename VT>
ff::PushCollector<T, VT>::PushCollector(VT& vec)
	: _vec(vec)
{
}

template<typename T, typename VT>
void ff::PushCollector<T, VT>::Push(T&& data)
{
	_vec.Push(std::move(data));
}

template<typename T, typename VT>
void ff::PushCollector<T, VT>::Push(const T& data)
{
	_vec.Push(data);
}

template<typename T, typename ST>
ff::SetCollector<T, ST>::SetCollector(ST& set)
	: _set(vec)
{
}

template<typename T, typename ST>
void ff::SetCollector<T, ST>::Push(T&& data)
{
	_set.SetKey(std::move(data));
}

template<typename T, typename ST>
void ff::SetCollector<T, ST>::Push(const T& data)
{
	_set.SetKey(data);
}
