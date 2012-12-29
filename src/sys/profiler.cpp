#include "sys/profiler.h"
#include <cstring>
#include <cstdio>

using namespace profiler;


namespace
{

	struct Timer {
		double value, avg;
		const char *id;
		bool auto_clear;
	};

	struct Counter {
		long long value;
		const char *id;
	};
	
	struct StackedTimer {
		const char *id;
		double start_time;
	};

	vector<StackedTimer> s_stack;

	vector<Timer> s_timers;
	vector<Counter> s_counters;
	int s_frame_count = 0;
	double s_rdtsc_multiplier = 1.0;
	double s_last_frame_time[2] = { -1.0, -1.0 };

}


namespace profiler {

	double rdtscTime() {
		unsigned long long val;
	    __asm__ __volatile__ ("rdtsc" : "=A" (val));
		return double(val) * 1.0e-9;
	}

	double rdtscMultiplier() {
		return s_rdtsc_multiplier;
	}

	void updateTimer(const char *id, double time, bool auto_clear) {
		for(int n = 0; n < (int)s_timers.size(); n++)
			if(s_timers[n].id == id) {
				s_timers[n].auto_clear = auto_clear;
				if(auto_clear)
					s_timers[n].value += time;
				else
					s_timers[n].value = time;
				return;
			}

		s_timers.push_back(Timer{time, 0.0, id, auto_clear});
	}

	void updateCounter(const char *id, int value) {
		for(int n = 0; n < (int)s_counters.size(); n++)
			if(s_counters[n].id == id) {
				s_counters[n].value += value;
				return;
			}

		s_counters.push_back(Counter{value, id});
	}

	void begin(const char *id) {
		s_stack.push_back(StackedTimer{id, 0.0f});
		s_stack.back().start_time = getTime();
	}

	void end(const char *id) {
		double time = getTime();
		DASSERT(!s_stack.empty() && s_stack.back().id == id);
		updateTimer(id, time - s_stack.back().start_time);
		s_stack.pop_back();
	}

	void nextFrame() {
		double current_time = getTime();
		double current_rtime = rdtscTime();

		if(s_last_frame_time[0] > 0.0) {
			double diff  = current_time  - s_last_frame_time[0];
			double rdiff = current_rtime - s_last_frame_time[1];
			s_rdtsc_multiplier = diff / rdiff;
		}

		s_last_frame_time[0] = current_time;
		s_last_frame_time[1] = current_rtime;

		for(int n = 0; n < (int)s_timers.size(); n++) if(s_timers[n].auto_clear) {
			s_timers[n].avg += s_timers[n].value;
			s_timers[n].value = 0.0;
		}
		for(int n = 0; n < (int)s_counters.size(); n++)
			s_counters[n].value = 0;
		s_frame_count++;
	}

	const string getStats(const char *filter) {
		char buffer[1024], *ptr = buffer, *end = buffer + sizeof(buffer);

		if(!s_timers.empty())
			ptr += snprintf(ptr, end - ptr, "Timers:\n");
		for(int n = 0; n < (int)s_timers.size(); n++) {
			const Timer &timer = s_timers[n];
			double ms = timer.value * s_rdtsc_multiplier * 1000.0;
			double us = ms * 1000.0;
			bool print_ms = ms > 0.5;
			ptr += snprintf(ptr, end - ptr, "  %s: %.2f %s\n", timer.id, print_ms? ms : us, print_ms? "ms" : "us");
		}

		if(!s_counters.empty())
			ptr += snprintf(ptr, end - ptr, "Counters:\n");
		for(int n = 0; n < (int)s_counters.size(); n++) {
			const Counter &counter = s_counters[n];
			if(strstr(counter.id, filter))
				ptr += snprintf(ptr, end - ptr, "  %s: %lld\n", counter.id, counter.value);
		}

		return buffer;
	}


}
