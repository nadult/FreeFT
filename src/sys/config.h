/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#include "base.h"

struct Config {
	Config() :resolution(1400, 768), time_multiplier(1.0f), fullscreen(false), profiler_enabled(false) { }

	int2 resolution;
	float time_multiplier;
	bool fullscreen;
	bool profiler_enabled;
};

Config loadConfig(const char *config_name);

#endif
