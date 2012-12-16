#ifndef SYS_PROFILER_H
#define SYS_PROFILER_H

#include "base.h"

namespace Profiler {

	void updateTimer(const char *name, double time, bool auto_clear = true);
	void updateCounter(const char *name, int value);
	void nextFrame();

	void begin(const char *name);
	void end(const char *name);

	const string getStats(const char *filter = "");

	struct AutoTimer {
		AutoTimer(const char *id, bool auto_clear) :time(getTime()), id(id), auto_clear(auto_clear) { }
		~AutoTimer() { updateTimer(id, getTime() - time, auto_clear); }

		double time;
		const char *id;
		bool auto_clear;
	};

#define PROFILE(id) Profiler::AutoTimer timer ## __LINE__(id, true)
#define PROFILE_RARE(id) Profiler::AutoTimer timer ## __LINE__(id, false)

};

#endif
