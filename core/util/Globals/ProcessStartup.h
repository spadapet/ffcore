#pragma once

namespace ff
{
	class ProcessGlobals;

	// Use static instances of this to hook into the startup of the program.
	struct ProcessStartup
	{
		typedef std::function<void(ProcessGlobals&)> FuncType;

		UTIL_API ProcessStartup(FuncType func);
		static void OnStartup(ProcessGlobals& globals);

		FuncType const _func;
		ProcessStartup* const _next;
	};

	// Use static instances of this to hook into the shutdown of the program.
	struct ProcessShutdown
	{
		typedef std::function<void(ProcessGlobals&)> FuncType;

		ProcessShutdown(FuncType func);
		static void OnShutdown(ProcessGlobals& globals);

		FuncType const _func;
		ProcessShutdown* const _next;
	};
}
