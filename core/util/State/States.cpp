#include "pch.h"
#include "State/States.h"
#include "State/StateWrapper.h"

ff::States::States()
{
}

ff::States::~States()
{
}

void ff::States::AddTop(std::shared_ptr<State> state)
{
	if (state != nullptr)
	{
		_states.Push(std::make_shared<StateWrapper>(state));
	}
}

void ff::States::AddBottom(std::shared_ptr<State> state)
{
	if (state != nullptr)
	{
		_states.InsertFirst(std::make_shared<StateWrapper>(state));
	}
}

std::shared_ptr<ff::State> ff::States::Advance(AppGlobals* globals)
{
	for (auto i = _states.GetFirst(); i; )
	{
		(*i)->Advance(globals);

		if ((*i)->GetStatus() == State::Status::Dead)
		{
			i = _states.Delete(*i);
		}
		else
		{
			i = _states.GetNext(*i);
		}
	}

	if (_states.Size() == 1)
	{
		return *_states.GetFirst();
	}

	return nullptr;
}

void ff::States::Render(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->Render(globals, target, depth);
	}
}

void ff::States::AdvanceInput(AppGlobals* globals)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->AdvanceInput(globals);
	}
}

void ff::States::AdvanceDebugInput(AppGlobals* globals)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->AdvanceDebugInput(globals);
	}
}

void ff::States::OnFrameStarted(AppGlobals* globals, AdvanceType type)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->OnFrameStarted(globals, type);
	}
}

void ff::States::OnFrameRendering(AppGlobals* globals, AdvanceType type)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->OnFrameRendering(globals, type);
	}
}

void ff::States::OnFrameRendered(AppGlobals* globals, AdvanceType type, IRenderTarget* target, IRenderDepth* depth)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->OnFrameRendered(globals, type, target, depth);
	}
}

void ff::States::SaveState(AppGlobals* globals)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->SaveState(globals);
	}
}

void ff::States::LoadState(AppGlobals* globals)
{
	for (const std::shared_ptr<State>& state : _states)
	{
		state->LoadState(globals);
	}
}

ff::State::Status ff::States::GetStatus()
{
	size_t ignoreCount = 0;
	size_t deadCount = 0;

	for (const std::shared_ptr<State>& state : _states)
	{
		switch (state->GetStatus())
		{
		case State::Status::Loading:
			return State::Status::Loading;

		case State::Status::Dead:
			deadCount++;
			break;

		case State::Status::Ignore:
			ignoreCount++;
			break;
		}
	}

	return (deadCount == _states.Size() - ignoreCount)
		? State::Status::Dead
		: Status::Alive;
}
