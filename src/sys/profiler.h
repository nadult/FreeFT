/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_PROFILER_H
#define SYS_PROFILER_H

#include "base.h"

//#define USE_RDTSC


namespace profiler {

	void updateTimer(const char *name, double time, bool auto_clear = true);
	void updateCounter(const char *name, int value);
	void nextFrame();

	void begin(const char *name);
	void end(const char *name);

	const string getStats(const char *filter = "");

#ifdef USE_RDTSC
	double rdtscTime();
	double rdtscMultiplier();
	inline double getTime() { return rdtscTime(); }
#else
	inline double getTime() { return baselib::getTime(); }
#endif

	struct AutoTimer {
#ifdef USE_RDTSC
		AutoTimer(const char *id, bool auto_clear) :time(rdtscTime()), id(id), auto_clear(auto_clear) { }
		~AutoTimer() { updateTimer(id, rdtscTime() - time, auto_clear); }
#else
		AutoTimer(const char *id, bool auto_clear) :time(getTime()), id(id), auto_clear(auto_clear) { }
		~AutoTimer() { updateTimer(id, getTime() - time, auto_clear); }
#endif

		double time;
		const char *id;
		bool auto_clear;
	};

#define PROFILE(id) profiler::AutoTimer PROFILE_NAME(__LINE__)(id, true)
#define PROFILE_RARE(id) profiler::AutoTimer PROFILE_NAME(__LINE__)(id, false)

#define PROFILE_NAME(a) PROFILE_NAME_(a)
#define PROFILE_NAME_(a) timer ## a
};

#endif
