#ifndef SYS_CONFIG_H
#define SYS_CONFIG_H

#include "base.h"

struct Config {
	Config() :resolution(1400, 768), fullscreen(false) { }

	int2 resolution;
	bool fullscreen;
};

Config loadConfig(const char *config_name);

#endif
