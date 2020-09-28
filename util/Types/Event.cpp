#include "pch.h"
#include "Types/Event.h"

ff::EventCookie ff::Event<void>::Add(FuncType&& func)
{
	return &*this->callbacks.emplace_after(this->callbacks.before_begin(), func);
}

void ff::Event<void>::Remove(EventCookie& cookie)
{
	if (cookie)
	{
		*(FuncType*)cookie = FuncType();
		cookie = nullptr;
	}
}

void ff::Event<void>::Notify()
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
			(*i)();
		}
	}
}
