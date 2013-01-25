/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "game/level.h"
#include "sys/xml.h"

namespace game {

	static vector<char> applyPatch(const vector<char> &orig, const vector<char> &patch) {
		//TODO: write me
		return orig;
	}

	Level::Level() :entity_map(tile_map) {
	}

	void Level::load(const char *file_name) {
		XMLDocument doc;

		int len = strlen(file_name);
		if(len > 4 && strcmp(file_name + len - 4, ".mod") == 0) {
			string orig_file_name(file_name, file_name + len - 4);
			orig_file_name += ".xml";

			vector<char> orig, mod; {
				Loader orig_sr(orig_file_name.c_str());
				Loader mod_sr(file_name);
				orig.resize(orig_sr.size());
				mod.resize(mod_sr.size());
				orig_sr.data(orig.data(), orig.size());
				mod_sr.data(mod.data(), mod.size());
			}

			vector<char> patched = applyPatch(orig, mod);

			Serializer sr(PStream(new DataStream(patched)), true);
			sr & doc;
		}
		else
			doc.load(file_name);

		tile_map.loadFromXML(doc);
		entity_map.loadFromXML(doc);
	}

	void Level::save(const char *file_name) const {
		XMLDocument doc;
		tile_map.saveToXML(doc);
		entity_map.saveToXML(doc);
		doc.save(file_name);
	}

}
