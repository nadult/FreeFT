// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include <fwk/gfx/gl_texture.h>
#include <fwk/hash_map.h>

#include "res_manager.h"

#include "game/tile.h"
#include <fwk/gfx/font.h>
#include <fwk/io/file_stream.h>
#include <fwk/io/file_system.h>

// TODO: fix error handling in all places with Ex<>
// TODO: dynamic not needed in s_tiles; But: TileFrame depending on CachedTexture
// causes problems in copy constructors & operator=
// TODO: there is a need for data structure which doesn't move resources in memory
// after thet are created
//
// TODO: dont return references but identifiers (TileId: u16)

ResManager *ResManager::g_instance = nullptr;

ResManager::ResManager() {
	ASSERT(g_instance == nullptr);
	g_instance = this;

	m_data_path = FilePath(executablePath()).parent() / "data";
	if(!m_data_path.ends_with('/'))
		m_data_path += '/';
}

ResManager::~ResManager() {
	ASSERT(g_instance == this);
	g_instance = nullptr;
}

PTexture ResManager::getTexture(Str str, HashMap<string, PTexture> &map, Str prefix, Str suffix) {
	auto it = map.find(str);
	if(it == map.end()) {
		auto file_name = format("%%%%", m_data_path, prefix, str, suffix);
		auto tex = GlTexture::load(file_name);
		tex.check();
		map.emplace(str, *tex);
		return *tex;
	}
	return it->value;
}

PTexture ResManager::getTexture(Str name) { return getTexture(name, m_textures, "", ""); }

PTexture ResManager::getGuiTexture(Str name) {
	return getTexture(name, m_gui_textures, "gui/", ".zar");
}

const Font &ResManager::getFont(Str name) {
	auto it = m_fonts.find(name);
	if(it == m_fonts.end()) {
		auto file_name = format("%%%%", m_data_path, "fonts/", name, ".fnt");
		auto vcore = FontCore::load(file_name);
		vcore.check();
		FontCore core(move(*vcore));
		auto tex = getTexture(core.textureName(), m_font_textures, "fonts/", "");
		it = m_fonts.emplace(name, Font(move(core), move(tex))).first;
	}
	return it->second;
}

const game::Tile &ResManager::getTile(Str name) {
	auto it = m_tiles.find(name);
	if(it == m_tiles.end()) {
		auto file_name = format("%%%%", m_data_path, "tiles/", name, ".tile");
		auto ldr = fileLoader(file_name);
		ldr.check();
		Dynamic<game::Tile> tile;
		tile.emplace();
		auto result = tile->load(*ldr);
		result.check();
		it = m_tiles.emplace(name, move(tile)).first;
	}
	return *it->second;
}

Pair<string> ResManager::tilePrefixSuffix() { return {m_data_path + "tiles/", ".tile"}; }

namespace res {
PTexture getTexture(Str name) { return ResManager::instance().getTexture(name); }
PTexture getGuiTexture(Str name) { return ResManager::instance().getGuiTexture(name); }
const Font &getFont(Str name) { return ResManager::instance().getFont(name); }
const game::Tile &getTile(Str name) { return ResManager::instance().getTile(name); }
}
