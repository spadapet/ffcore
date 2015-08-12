#include "pch.h"
#include "State/State.h"

ff::State::State()
{
}

ff::State::~State()
{
}

std::shared_ptr<ff::State> ff::State::Advance(AppGlobals* globals)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->Advance(globals);
	}

	return nullptr;
}

void ff::State::Render(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->Render(globals, target, depth);
	}
}

void ff::State::AdvanceInput(AppGlobals* globals)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->AdvanceInput(globals);
	}
}

void ff::State::AdvanceDebugInput(AppGlobals* globals)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->AdvanceDebugInput(globals);
	}
}

void ff::State::OnFrameStarted(AppGlobals* globals, AdvanceType type)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->OnFrameStarted(globals, type);
	}
}

void ff::State::OnFrameRendering(AppGlobals* globals, AdvanceType type)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->OnFrameRendering(globals, type);
	}
}

void ff::State::OnFrameRendered(AppGlobals* globals, AdvanceType type, IRenderTarget* target, IRenderDepth* depth)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->OnFrameRendered(globals, type, target, depth);
	}
}

void ff::State::SaveState(AppGlobals* globals)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->SaveState(globals);
	}
}

void ff::State::LoadState(AppGlobals* globals)
{
	for (size_t i = 0; i < GetChildStateCount(); i++)
	{
		GetChildState(i)->LoadState(globals);
	}
}

ff::State::Status ff::State::GetStatus()
{
	return Status::Alive;
}

size_t ff::State::GetChildStateCount()
{
	return 0;
}

ff::State* ff::State::GetChildState(size_t index)
{
	return nullptr;
}
