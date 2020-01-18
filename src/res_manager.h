// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"

#include <fwk/fwd_member.h>
#include <map>

// Only single instance allowed
class ResManager {
  public:
	ResManager();
	~ResManager();

	ResManager(const ResManager &) = delete;
	void operator=(ResManager &) = delete;

	static ResManager &instance() {
		DASSERT(g_instance);
		return *g_instance;
	}

	PTexture getTexture(Str);
	PTexture getGuiTexture(Str);
	const Font &getFont(Str);
	const game::Tile &getTile(Str);
	const auto &allTiles() { return m_tiles; }
	
	static pair<Str, Str> tilePrefixSuffix();

  private:
	PTexture getTexture(Str str, HashMap<string, PTexture> &map, Str prefix, Str suffix);

	static ResManager *g_instance;

	FwdMember<HashMap<string, PTexture>> m_gui_textures, m_textures;
	FwdMember<HashMap<string, PTexture>> m_font_textures;
	std::map<string, Dynamic<game::Tile>> m_tiles;
	std::map<string, Font> m_fonts;
};
