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
			config.resolution.x = node.intAttrib("res_x");
			config.resolution.y = node.intAttrib("res_y");
			config.fullscreen = node.intAttrib("fullscreen") != 0;	
			config.profiler_enabled = node.intAttrib("profiler") != 0;
			config.time_multiplier = node.floatAttrib("time_multiplier");
		}
	}
	catch(...) { }

	//TODO: if no configuration file is present, then
	// it should be created with a default values

	return config;
}
