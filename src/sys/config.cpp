// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "sys/config.h"

#include <fwk/io/file_system.h>

Config::Config() : resolution(1280, 720), window_pos(0, 0), fullscreen_on(false), profiler_on(false) {}

Config::Config(CXmlNode node) : Config() { load(node); }

Config::Config(const char *config_name) : Config() {
	// TODO: load XML files through ResManager?
	auto file_name = "data/config.xml";
	if(auto doc = XmlDocument::load(file_name)) {
		auto node = doc->child(config_name);
		if(!node)
			node = doc->child("default");
		if(node)
			load(node);
	}
}

void Config::load(CXmlNode node) {
	DASSERT(node);
	resolution = node.attrib<int2>("res", resolution);
	window_pos = node.attrib<int2>("window_pos", window_pos);
	fullscreen_on = node.attrib<bool>("fullscreen", fullscreen_on);
	profiler_on = node.attrib<bool>("profiler", profiler_on);
}
