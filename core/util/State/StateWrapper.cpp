#include "pch.h"
#include "State/StateWrapper.h"

ff::StateWrapper::StateWrapper()
{
}

ff::StateWrapper::StateWrapper(std::shared_ptr<ff::State> state)
{
	SetState(nullptr, state);
}

ff::StateWrapper::~StateWrapper()
{
}

ff::StateWrapper& ff::StateWrapper::operator=(const std::shared_ptr<State>& state)
{
	SetState(nullptr, state);
	return *this;
}

std::shared_ptr<ff::State> ff::StateWrapper::Advance(AppGlobals* globals)
{
	noAssertRetVal(_state != nullptr, nullptr);

	SetState(globals, _state->Advance(globals));

	if (_state->GetStatus() == State::Status::Dead)
	{
		_state->SaveState(globals);
		_state = nullptr;
	}

	return nullptr;
}

void ff::StateWrapper::Render(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth)
{
	noAssertRet(_state != nullptr);
	_state->Render(globals, target, depth);
}

void ff::StateWrapper::AdvanceInput(AppGlobals* globals)
{
	noAssertRet(_state != nullptr);
	_state->AdvanceInput(globals);
}

void ff::StateWrapper::AdvanceDebugInput(AppGlobals* globals)
{
	noAssertRet(_state != nullptr);
	_state->AdvanceDebugInput(globals);

}

void ff::StateWrapper::OnFrameStarted(AppGlobals* globals, AdvanceType type)
{
	noAssertRet(_state != nullptr);
	_state->OnFrameStarted(globals, type);
}

void ff::StateWrapper::OnFrameRendering(AppGlobals* globals, AdvanceType type)
{
	noAssertRet(_state != nullptr);
	_state->OnFrameRendering(globals, type);
}

void ff::StateWrapper::OnFrameRendered(AppGlobals* globals, AdvanceType type, IRenderTarget* target, IRenderDepth* depth)
{
	noAssertRet(_state != nullptr);
	_state->OnFrameRendered(globals, type, target, depth);
}

void ff::StateWrapper::SaveState(AppGlobals* globals)
{
	noAssertRet(_state != nullptr);
	_state->SaveState(globals);
}

void ff::StateWrapper::LoadState(AppGlobals* globals)
{
	noAssertRet(_state != nullptr);
	_state->LoadState(globals);
}

ff::State::Status ff::StateWrapper::GetStatus()
{
	noAssertRetVal(_state != nullptr, State::Status::Dead);
	return _state->GetStatus();
}

void ff::StateWrapper::SetState(AppGlobals* globals, const std::shared_ptr<State>& state)
{
	if (state != nullptr && state != _state)
	{
		if (globals)
		{
			_state->SaveState(globals);
		}

		_state = state;

		if (globals)
		{
			_state->LoadState(globals);
		}

		CheckState();
	}
}

void ff::StateWrapper::CheckState()
{
	// Don't need nested wrappers
	while (true)
	{
		std::shared_ptr<StateWrapper> wrapper = std::dynamic_pointer_cast<StateWrapper>(_state);
		if (wrapper == nullptr)
		{
			break;
		}

		_state = wrapper->_state;
	}
}
