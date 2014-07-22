/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#include "base.h"

struct Config {
	Config() :resolution(1400, 768), window_pos(0, 0), fullscreen(false), profiler_enabled(false) { }

	int2 resolution;
	int2 window_pos;
	bool fullscreen;
	bool profiler_enabled;
};

Config loadConfig(const char *config_name);

#endif
