#pragma once

namespace ff
{
	struct GlobalTime
	{
		UTIL_API GlobalTime();

		size_t _frameCount;
		size_t _advanceCount;
		size_t _renderCount;
		size_t _advancesPerSecond;
		double _advancesPerSecondD;
		double _appSeconds;
		double _clockSeconds;
		double _secondsPerAdvance;
		double _bankSeconds; // the bank is extra time that wasn't used in a call to Advance()
		double _bankScale; // From 0 to 1, the amount of the next frame that's in the bank
		double _timeScale;
	};

	struct FrameTime
	{
		UTIL_API FrameTime();

		static const size_t MAX_ADVANCE_COUNT = 4;
		static const size_t MAX_ADVANCE_MULTIPLIER = 4;

		GraphCounters _graphCounters;
		size_t _advanceCount;
		std::array<INT64, MAX_ADVANCE_COUNT * MAX_ADVANCE_MULTIPLIER> _advanceTime;
		INT64 _renderTime;
		INT64 _flipTime;

		INT64 _freq;
		double _freqD;
	};

	// Wrapper for QueryPerformanceCounter and keeping track of frames per second
	class UTIL_API Timer
	{
	public:
		Timer();
		~Timer();

		double Tick(double forcedOffset = -1.0); // Update the time
		void Reset(); // Start counting seconds from zero

		double GetSeconds() const; // seconds passed since the last Reset, affected by SetTimeScale
		double GetTickSeconds() const; // seconds passed since the last Tick
		double GetClockSeconds() const; // seconds passed since the last Reset, not affected by SetTimeScale
		size_t GetNumTicks() const; // number of times Tick() was called
		size_t GetTicksPerSecond() const; // number of times Tick() was called during the last second

		double GetTimeScale() const;
		void SetTimeScale(double scale);

		INT64 GetLastTickRawTime() const;
		static INT64 GetCurrentRawTime();
		static INT64 GetRawFreqStatic();
		INT64 GetRawFreq() const;
		double GetRawFreqD() const;

		void StoreLastTickTime();
		INT64 GetLastTickStoredRawTime(); // updates the stored start time too
		INT64 GetCurrentStoredRawTime(); // updates the stored start time too

	private:
		INT64 _resetTime;
		INT64 _startTime;
		INT64 _curTime;
		INT64 _storedTime;
		INT64 _freq;

		size_t _numTicks;
		size_t _tpsSecond;
		size_t _tpsCurSecond;
		size_t _tpsCount;
		size_t _tps;

		double _timeScale;
		double _startSeconds;
		double _seconds;
		double _clockSeconds;
		double _freqDouble;
		double _passSec;
	};
}
