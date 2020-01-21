// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include <fwk/gfx/gl_texture.h>
#include <fwk/hash_map.h>

#include "res_manager.h"

#include "game/tile.h"
#include <fwk/gfx/font.h>
#include <fwk/gfx/texture.h>
#include <fwk/io/file_stream.h>
#include <fwk/io/file_system.h>
#include <fwk/io/memory_stream.h>

// TODO: fix error handling in all places with Ex<>
// TODO: dynamic not needed in s_tiles; But: TileFrame depending on CachedTexture
// causes problems in copy constructors & operator=
// TODO: there is a need for data structure which doesn't move resources in memory
// after thet are created
//
// TODO: dont return references but identifiers (TileId: u16)

ResManager *ResManager::g_instance = nullptr;

static const EnumMap<ResType, Pair<const char*>> default_paths = {{
	{ResType::tile, {"tiles/", ".tile"}},
	{ResType::sprite, {"sprites/", ".sprite"}},
	{ResType::font, {"fonts/", ".fnt"}},
	{ResType::texture, {"", ""}},
}};

ResManager::ResManager(bool absolute_paths) {
	ASSERT(g_instance == nullptr);
	g_instance = this;

	if(absolute_paths) {
		m_data_path = FilePath(executablePath()).parent() / "data";
		if(!m_data_path.ends_with('/'))
			m_data_path += '/';
	}
	else {
		m_data_path = "data/";
	}

	for(auto rtype : all<ResType>)
		m_paths[rtype] = {m_data_path + default_paths[rtype].first, default_paths[rtype].second};
}

ResManager::~ResManager() {
	ASSERT(g_instance == this);
	g_instance = nullptr;
}

PTexture ResManager::getTexture(Str name) {
	auto it = m_textures.find(name);
	if(it == m_textures.end()) {
		auto tex = GlTexture::load(fullPath(name, ResType::texture));
		tex.check();
		m_textures.emplace(name, *tex);
		return *tex;
	}
	return it->value;
}

const Font &ResManager::getFont(Str name) {
	auto it = m_fonts.find(name);
	if(it == m_fonts.end()) {
		auto vcore = FontCore::load(fullPath(name, ResType::font));
		vcore.check();
		FontCore core(move(*vcore));
		auto tex = getTexture("fonts/" + core.textureName());
		it = m_fonts.emplace(name, Font(move(core), move(tex))).first;
	}
	return it->second;
}

const game::Tile &ResManager::getTile(Str name) {
	auto it = m_tiles.find(name);
	if(it == m_tiles.end()) {
		auto ldr = fileLoader(fullPath(name, ResType::tile));
		ldr.check();
		Dynamic<game::Tile> tile;
		tile.emplace();
		auto result = tile->load(*ldr);
		result.check();
		it = m_tiles.emplace(name, move(tile)).first;
	}
	return *it->second;
}
	
Ex<void> ResManager::loadResource(Str name, Stream &sr, ResType type) {
	DASSERT(sr.isLoading());

	if(type == ResType::tile) {
		Dynamic<game::Tile> tile;
		tile.emplace();
		EXPECT(tile->load(sr));
		m_tiles[name] = move(tile);
	}
	else if(type == ResType::sprite) {
		FATAL("write me");
	}
	else if(type == ResType::font) {
		auto tex = getTexture(format("fonts/%", name));
		auto doc = EX_PASS(XmlDocument::load(sr));
		auto core = EX_PASS(FontCore::load(doc));
		m_fonts.erase(name);
		m_fonts.emplace(name, Font(move(core), move(tex)));
	}
	else if(type == ResType::texture) {
		auto ext = fileExtension(name);
		if(!ext)
			return ERROR("Texture without extension: '%'", name);
		auto tex = EX_PASS(Texture::load(sr, *ext));
		m_textures[name] = GlTexture::make(tex);
	}

	return {};
}
	
Ex<void> ResManager::loadResource(Str name, CSpan<char> data, ResType type) {
	auto mem_loader = memoryLoader(data);
	return loadResource(name, mem_loader, type);
}

Maybe<ResType> ResManager::classifyPath(Str path, bool ignore_prefix) const {
	if(path.endsWith(".zar") || path.endsWith(".png") || path.endsWith(".tga")) {
		if(ignore_prefix || path.startsWith(m_data_path))
			return ResType::texture;
		return none;
	}

	for(auto rtype : ~ResType::texture) {
		auto &paths = m_paths[rtype];
		if(path.endsWith(paths.second) && (ignore_prefix || path.startsWith(paths.first)))
			return rtype;
	}
	return none;
}

Maybe<string> ResManager::resourceName(Str path, ResType type) const {
	auto &paths = m_paths[type];
	string out = path;
	if(!removePrefix(out, paths.first))
		return none;
	if(!removePrefix(out, paths.second))
		return none;
	return out;
}
	
string ResManager::fullPath(Str res_name, ResType type) const {
	auto &paths = m_paths[type];
	return format("%%%", paths.first, res_name, paths.second);
}

namespace res {
PTexture getTexture(Str name) { return ResManager::instance().getTexture(name); }
PTexture getGuiTexture(Str name) { return ResManager::instance().getTexture(format("gui/%.zar", name)); }
const Font &getFont(Str name) { return ResManager::instance().getFont(name); }
const game::Tile &getTile(Str name) { return ResManager::instance().getTile(name); }
}
