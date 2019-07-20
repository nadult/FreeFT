// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

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
