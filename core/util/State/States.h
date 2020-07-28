#pragma once

#include "State/State.h"

namespace ff
{
	// Deals with a list of State objects that all advance and render together
	class States : public ff::State
	{
	public:
		UTIL_API States();
		virtual ~States();

		UTIL_API void AddTop(std::shared_ptr<State> state);
		UTIL_API void AddBottom(std::shared_ptr<State> state);

		virtual std::shared_ptr<State> Advance(AppGlobals* globals) override;
		virtual void Render(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth) override;
		virtual void AdvanceInput(AppGlobals* globals) override;
		virtual void AdvanceDebugInput(AppGlobals* globals) override;
		virtual void OnFrameStarted(AppGlobals* globals, AdvanceType type) override;
		virtual void OnFrameRendering(AppGlobals* globals, AdvanceType type) override;
		virtual void OnFrameRendered(AppGlobals* globals, AdvanceType type, IRenderTarget* target, IRenderDepth* depth) override;
		virtual void SaveState(AppGlobals* globals) override;
		virtual void LoadState(AppGlobals* globals) override;
		virtual Status GetStatus() override;
		virtual Cursor GetCursor() override;

	private:
		ff::List<std::shared_ptr<State>> _states;
	};
}
