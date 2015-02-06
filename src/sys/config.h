/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#include "base.h"

struct Config {
	Config();
	Config(const XMLNode&);
	Config(const char *config_name);

	void load(const XMLNode&);

	int2 resolution;
	int2 window_pos;
	bool fullscreen_on;
	bool profiler_on;
};

#endif
