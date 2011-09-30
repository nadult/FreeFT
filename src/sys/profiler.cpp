#include "sys/profiler.h"
#include <cstring>

using namespace Profiler;


namespace
{
	const char *counterNames[numCounters] = {
		"Rendered nodes",
		"Rendered tiles",
	};

	const char *timerNames[numTimers] = {
		"Rendering time",
		"Rendering preparation time",
	};

	int counters[numCounters] = {0, };
	double timers[numTimers] = {0.0, };
}


namespace Profiler {

	void UpdateTimer(TimerId id, double time) {
		timers[id] += time;
	}

	void UpdateCounter(CounterId id, int value) {
		counters[id] += value;
	}

	void NextFrame() {
		memset(counters, 0, sizeof(counters));
		memset(timers, 0, sizeof(timers));
	}

	string GetStats() {
		char buffer[1024], *ptr = buffer, *end = buffer + sizeof(buffer);

		ptr += snprintf(ptr, end - ptr, "Timers:\n");
		for(int n = 0; n < numTimers; n++)
			ptr += snprintf(ptr, end - ptr, "  %s: %.2f ms\n", timerNames[n], timers[n] * 1000.0);


		ptr += snprintf(ptr, end - ptr, "Counters:\n");
		for(int n = 0; n < numCounters; n++)
			ptr += snprintf(ptr, end - ptr, "  %s: %d\n", counterNames[n], counters[n]);

		return buffer;
	}


}
