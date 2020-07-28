#pragma once

#include "State/State.h"

namespace ff
{
	// Automatically updates the state object based on requests from its Advance() call
	class StateWrapper : public ff::State
	{
	public:
		UTIL_API StateWrapper();
		UTIL_API StateWrapper(std::shared_ptr<State> state);
		virtual ~StateWrapper();

		UTIL_API StateWrapper& operator=(const std::shared_ptr<State>& state);

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
		void SetState(AppGlobals* globals, const std::shared_ptr<State>& state);
		void CheckState();

		std::shared_ptr<ff::State> _state;
	};
}
