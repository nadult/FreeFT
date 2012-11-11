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

	void updateTimer(TimerId id, double time);
	void updateCounter(CounterId id, int value);
	void nextFrame();

	string getStats();

	struct BlockTimer {
		BlockTimer(TimerId id) :time(getTime()), id(id) { }
		~BlockTimer() { updateTimer(id, getTime() - time); }

		double time;
		TimerId id;
	};

#define PROFILE(id) Profiler::BlockTimer timer ## id(Profiler::id);

};

#endif
