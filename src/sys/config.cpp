/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/config.h"
#include "sys/xml.h"

	
Config::Config()
	:resolution(0, 0), window_pos(0, 0), fullscreen_on(false), profiler_on(false) {
}

Config::Config(const XMLNode &node) :Config() {
	load(node);
}
	
Config::Config(const char *config_name) :Config() {
	XMLDocument doc;
	doc.load("data/config.xml");
		
	XMLNode node = doc.child(config_name);
	if(!node)
		node = doc.child("default");
	if(node)
		load(node);
}

void Config::load(const XMLNode &node) {
	DASSERT(node);

	if(auto *res_value = node.hasAttrib("res"))
		resolution = toInt2(res_value);
	if(auto *window_pos_value = node.hasAttrib("window_pos"))
		window_pos = toInt2(window_pos_value);
	if(auto *fullscreen_value = node.hasAttrib("fullscreen"))
		fullscreen_on = toBool(fullscreen_value);
	if(auto *profiler_value = node.hasAttrib("profiler"))
		profiler_on = toBool(profiler_value);
}
