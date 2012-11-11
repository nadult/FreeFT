#ifndef SYS_PROFILER_H
#define SYS_PROFILER_H

#include "base.h"

namespace Profiler {

	enum CounterId {
		cRenderedNodes = 0,
		cRenderedTiles,

		numCounters,
	};

	enum TimerId {
		tRendering = 0,
		tRenderingPreparation,

		numTimers,
	};

	void UpdateTimer(TimerId id, double time);
	void UpdateCounter(CounterId id, int value);
	void NextFrame();

	string GetStats();

	struct BlockTimer {
		BlockTimer(TimerId id) :time(getTime()), id(id) { }
		~BlockTimer() { UpdateTimer(id, getTime() - time); }

		double time;
		TimerId id;
	};

#define PROFILE(id) Profiler::BlockTimer timer ## id(Profiler::id);

};

#endif
