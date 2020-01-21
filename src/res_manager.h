// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"

#include <fwk/fwd_member.h>
#include <map>

DEFINE_ENUM(ResType, tile, sprite, font, texture);

// Only single instance allowed
class ResManager {
  public:
	ResManager(bool absolute_paths = true);
	~ResManager();

	ResManager(const ResManager &) = delete;
	void operator=(ResManager &) = delete;

	static ResManager &instance() {
		DASSERT(g_instance);
		return *g_instance;
	}

	PTexture getTexture(Str);
	const Font &getFont(Str);
	const game::Tile &getTile(Str);
	const auto &allTiles() { return m_tiles; }
	
	Ex<void> loadResource(Str name, Stream&, ResType);
	Ex<void> loadResource(Str name, CSpan<char> data, ResType);

	Maybe<ResType> classifyPath(Str, bool ignore_prefix = false) const;
	// Will return none if path doesn't match resource prefix & suffix
	Maybe<string> resourceName(Str path, ResType) const;
	string fullPath(Str res_name, ResType) const;

	Pair<Str> prefixSuffix(ResType type) const { return m_paths[type]; }

  private:
	static ResManager *g_instance;

	EnumMap<ResType, Pair<string, string>> m_paths;
	string m_data_path;
	FwdMember<HashMap<string, PTexture>>  m_textures;
	std::map<string, Dynamic<game::Tile>> m_tiles;
	std::map<string, Font> m_fonts;
};
