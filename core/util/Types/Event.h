#pragma once

namespace ff
{
	typedef void* EventCookie;

	template<typename... Args>
	class Event
	{
	public:
		typedef std::function<void(Args...)> FuncType;

		EventCookie Add(FuncType&& func);
		void Remove(EventCookie& cookie);
		void Notify(Args... args);

	private:
		std::forward_list<FuncType> callbacks;
	};

	template<>
	class Event<void>
	{
	public:
		typedef std::function<void()> FuncType;

		UTIL_API EventCookie Add(FuncType&& func);
		UTIL_API void Remove(EventCookie& cookie);
		UTIL_API void Notify();

	private:
		std::forward_list<FuncType> callbacks;
	};
}

template<typename... Args>
ff::EventCookie ff::Event<Args...>::Add(FuncType&& func)
{
	return &*this->callbacks.emplace_after(this->callbacks.before_begin(), func);
}

template<typename... Args>
void ff::Event<Args...>::Remove(EventCookie& cookie)
{
	if (cookie)
	{
		*(FuncType*)cookie = FuncType();
		cookie = nullptr;
	}
}

template<typename... Args>
void ff::Event<Args...>::Notify(Args... args)
{
	for (auto prev = this->callbacks.before_begin(), i = this->callbacks.begin(); i != this->callbacks.end(); prev = i++)
	{
		if (*i == nullptr)
		{
			this->callbacks.erase_after(prev);
			i = prev;
		}
		else
		{
			(*i)(args...);
		}
	}
}
