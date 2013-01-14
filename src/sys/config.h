#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#include "base.h"

struct Config {
	Config() :resolution(1400, 768), fullscreen(false), profiler_enabled(false) { }

	int2 resolution;
	bool fullscreen;
	bool profiler_enabled;
};

Config loadConfig(const char *config_name);

#endif
