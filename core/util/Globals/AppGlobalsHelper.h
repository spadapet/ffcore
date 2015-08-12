#pragma once

namespace ff
{
	class AppGlobals;
	class State;

	enum class AdvanceType
	{
		Running,
		SingleStep,
		Stopped,
	};

	class IAppGlobalsHelper
	{
	public:
		UTIL_API virtual ~IAppGlobalsHelper();

		UTIL_API virtual bool OnInitialized(AppGlobals* globals);
		UTIL_API virtual void OnShutdown(AppGlobals* globals);
		UTIL_API virtual void OnGameThreadInitialized(AppGlobals* globals);
		UTIL_API virtual void OnGameThreadShutdown(AppGlobals* globals);
		UTIL_API virtual std::shared_ptr<State> CreateInitialState(AppGlobals* globals);
		UTIL_API virtual double GetTimeScale(AppGlobals* globals);
		UTIL_API virtual AdvanceType GetAdvanceType(AppGlobals* globals);
		UTIL_API virtual String GetWindowName();
	};
}
