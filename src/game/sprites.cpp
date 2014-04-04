/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/sprite.h"
#include "sys/platform.h"
#include <cstring>
#include <cstdio>
#include "sys/platform.h"

using namespace gfx;
using namespace sys;

namespace game {

	namespace {
		const char *s_prefix = "data/sprites/";
		const char *s_suffix = ".sprite";

		std::map<string, int> s_sprite_map;
		vector<Sprite> s_sprites;
	}

	static void loadSprite(int idx, bool full) {
		DASSERT(idx >= 0 && idx < (int)s_sprites.size());
		Sprite &sprite = s_sprites[idx];

		char file_name[1024];
		snprintf(file_name, sizeof(file_name), "%s%s%s", s_prefix, sprite.resourceName(), s_suffix);
		Loader ldr(file_name);
		sprite.load(ldr, full);
	}

	void Sprite::initMap() {
		if(!s_sprite_map.empty())
			return;

		vector<FileEntry> file_entries;
		findFiles(file_entries, s_prefix, FindFiles::regular_file | FindFiles::recursive);

		string suffix = s_suffix;
		int sprite_count = 0;

		for(int n = 0; n < (int)file_entries.size(); n++) {
			string name = (string)file_entries[n].path;
			removePrefix(name, s_prefix);

			if(removeSuffix(name, suffix))
				s_sprite_map.emplace(name, sprite_count++);
		}

		s_sprites.resize(sprite_count);
		for(auto it = s_sprite_map.begin(); it != s_sprite_map.end(); ++it) {
			s_sprites[it->second].setResourceName(it->first.c_str());
			s_sprites[it->second].setIndex(it->second);
			loadSprite(it->second, false);
		}

	}

	int Sprite::count() {
		return (int)s_sprites.size();
	}

	int Sprite::find(const string &name) {
		auto it = s_sprite_map.find(name);
		return it == s_sprite_map.end()? -1 : it->second;
	}

	const Sprite &Sprite::get(int idx) {
		DASSERT(isValidIndex(idx));
		Sprite &sprite = s_sprites[idx];
		if(!sprite.isFullyLoaded())
			loadSprite(idx, true);
		return sprite;
	}

	const Sprite &Sprite::get(const string &name) {
		int idx = find(name);
		if(idx == -1)
			THROW("Sprite not found: %s", name.c_str());
		return get(idx);
	}
		
	const Sprite &Sprite::getPartial(const string &name) {
		int idx = find(name);
		if(idx == -1)
			THROW("Sprite not found: %s", name.c_str());
		return s_sprites[idx];
	}
		
	bool Sprite::isValidIndex(int idx) {
		return idx >= 0 && idx < (int)s_sprites.size();
	}

}
