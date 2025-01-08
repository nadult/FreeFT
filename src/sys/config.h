// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"

struct Config {
	Config(CXmlNode);
	Config(const char *config_name);

	void load(CXmlNode);

	int2 resolution = {1280, 720};
	int2 window_pos = {100, 100};
	bool fullscreen_on = false;
	bool profiler_on = false;
};
