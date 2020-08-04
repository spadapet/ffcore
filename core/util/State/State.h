#pragma once

namespace ff
{
	class AppGlobals;
	class IRenderDepth;
	class IRenderTarget;
	enum class AdvanceType;

	// A State will advance 60hz and render up to 60fps
	class State : public std::enable_shared_from_this<State>
	{
	public:
		UTIL_API State();
		UTIL_API virtual ~State();

		UTIL_API virtual std::shared_ptr<State> Advance(AppGlobals* globals);
		UTIL_API virtual void Render(AppGlobals* globals, IRenderTarget* target, IRenderDepth* depth);

		UTIL_API virtual void AdvanceInput(AppGlobals* globals);
		UTIL_API virtual void OnFrameStarted(AppGlobals* globals, AdvanceType type);
		UTIL_API virtual void OnFrameRendering(AppGlobals* globals, AdvanceType type);
		UTIL_API virtual void OnFrameRendered(AppGlobals* globals, AdvanceType type, IRenderTarget* target, IRenderDepth* depth);

		UTIL_API virtual void SaveState(AppGlobals* globals);
		UTIL_API virtual void LoadState(AppGlobals* globals);

		enum class Status { Loading, Alive, Dead, Ignore };
		UTIL_API virtual Status GetStatus();

		enum class Cursor { Default, Hand };
		UTIL_API virtual Cursor GetCursor();

		UTIL_API virtual size_t GetChildStateCount();
		UTIL_API virtual State* GetChildState(size_t index);
	};
}
