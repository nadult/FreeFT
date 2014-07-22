/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "sys/config.h"
#include "sys/xml.h"

Config loadConfig(const char *name) {
	Config config;

	try {
		XMLDocument doc;
		doc.load("data/config.xml");

		XMLNode node = doc.child(name);
		if(!node)
			node = doc.child("default");

		if(node) {
			config.resolution = node.int2Attrib("res");
			config.window_pos = node.int2Attrib("window_pos");
			config.fullscreen = node.intAttrib("fullscreen") != 0;	
			config.profiler_enabled = node.intAttrib("profiler") != 0;
		}
	}
	catch(...) { }

	//TODO: if no configuration file is present, then
	// it should be created with a default values

	return config;
}
