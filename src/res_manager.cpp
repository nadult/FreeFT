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
#include <fwk/io/url_fetch.h>
#include <fwk/io/gzip_stream.h>

#ifdef FWK_PLATFORM_HTML
#include <emscripten.h>
#endif

// This file should be automatically generated
#include "res_embedded.cpp"

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
	{ResType::other, {"", ""}},
}};

ResManager::ResManager(bool console_mode) :m_console_mode(console_mode) {
	ASSERT(g_instance == nullptr);
	g_instance = this;

	if(platform != Platform::html) {
		m_data_path = FilePath(executablePath()).parent() / "data";
		if(!m_data_path.ends_with('/'))
			m_data_path += '/';
	}
	else {
		m_data_path = "data/";
	}

	for(auto rtype : all<ResType>)
		m_paths[rtype] = {m_data_path + default_paths[rtype].first, default_paths[rtype].second};

	// TODO: these packages should be loaded through main loop
	preloadEmbedded();
	if(platform == Platform::html)
		preloadPackages();
}

ResManager::~ResManager() {
	ASSERT(g_instance == this);
	g_instance = nullptr;
}

void fixGrayTransTexture(Texture &tex) {
	for(int n : intRange(tex.pixelCount()))
		tex[n] = IColor(u8(255), u8(255), u8(255), tex[n].r);
}

PTexture ResManager::getTexture(Str name, bool font_tex) {
	auto it = m_textures.find(name);
	if(it == m_textures.end()) {
		auto tex = Texture::load(fullPath(name, ResType::texture));
		tex.check();
		if(font_tex)
			fixGrayTransTexture(*tex);
		auto gl_tex = GlTexture::make(*tex);
		m_textures.emplace(name, gl_tex);
		return gl_tex;
	}
	return it->value;
}

const Font &ResManager::getFont(Str name) {
	auto it = m_fonts.find(name);
	if(it == m_fonts.end()) {
		auto vcore = FontCore::load(fullPath(name, ResType::font));
		vcore.check();
		FontCore core(move(*vcore));
		auto tex = getTexture("fonts/" + core.textureName(), true);
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
		tile->setResourceName(name);
		result.check();
		it = m_tiles.emplace(name, move(tile)).first;
	}
	return *it->second;
}
	
vector<char> ResManager::getOther(Str name)  const{
	auto it = m_others.find(name);
	if(it == m_others.end()) {
		auto data = loadFile(format("%%", m_data_path, name));
		data.check();
		return move(*data);
	}
	return it->second;
}
	
Ex<void> ResManager::loadResource(Str name, Stream &sr, ResType type) {
	DASSERT(sr.isLoading());

	if(type == ResType::tile) {
		Dynamic<game::Tile> tile;
		tile.emplace();
		EXPECT(tile->load(sr));
		tile->setResourceName(name);
		m_tiles[name] = move(tile);
	}
	else if(type == ResType::sprite) {
		FATAL("write me");
	}
	else if(type == ResType::font) {
		auto tex = getTexture(format("fonts/%", name), true);
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
	else if(type == ResType::other) {
		vector<char> data(sr.size());
		sr.loadData(data);
		EX_CATCH();
		m_others[name] = move(data);
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
	
Ex<void> ResManager::loadPackage(Str name, Str prefix) {
#ifdef FWK_PLATFORM_HTML
	print("Loading package: %\n", name);

	// TODO: setup timeout ?
	auto fetch = EX_PASS(UrlFetch::make(format("res/%.pack.gz", name)));	

	auto fetch_func = [](void *arg) {
		auto &fetch = *(UrlFetch*)arg;
		auto [a, b] = fetch.progress();
		auto status = fetch.status();
		print("Progress: % / % status:%\n", a, b, status);
		if(status != UrlFetchStatus::downloading)
			emscripten_cancel_main_loop();
		print("here...\n");
	};

	emscripten_set_main_loop_arg(fetch_func, &fetch, 15, 1);
	print("FInished loop!\n");
	return {};

	vector<char> data; {
		auto gz_data = EX_PASS(UrlFetch::finish(move(fetch)));
		auto mem_loader = memoryLoader(move(gz_data));
		auto gz_loader = EX_PASS(GzipStream::loader(mem_loader));
		data = EX_PASS(gz_loader.loadData());
	}

	print("Unpacked data: %\n", data.size());
	return {};
#else
	return ERROR("Not supported yet");
#endif
}

void ResManager::preloadEmbedded() {
#define TEX(prefix, name, fix_trans) \
	{prefix #name "_0.png", cspan(name##_0_png, name##_0_png_len).template reinterpret<char>(), fix_trans}
	struct Tex {
		Str name;
		CSpan<char> data;
		bool fix_trans;
	};

	Tex textures[] = {
		TEX("fonts/", liberation_16, true),
		TEX("fonts/", liberation_24, true),
		TEX("fonts/", liberation_32, true),
		TEX("fonts/", liberation_48, true),
		TEX("fonts/", transformers_20, true),
		TEX("fonts/", transformers_30, true),
		TEX("fonts/", transformers_48, true),
	};
#undef TEX

	if(!m_console_mode) for(auto [name, data, fix] : textures) {
		auto ldr = memoryLoader(data);
		auto tex = Texture::load(ldr, TextureFileType::png);
		tex.check();
		if(fix)
			fixGrayTransTexture(*tex);
		m_textures[name] = GlTexture::make(*tex);
	}
}

void ResManager::preloadPackages() {
	Pair<const char*> packages[] = {
		{"data", "data/"},
		{"fonts", "data/fonts/"},
		{"gui", "data/gui/"},
		{"sprites", "data/sprites/"},
		{"tiles", "data/tiles/"},
	};

	// TODO: UI with progress bar should be available while data is loading ?
	for(auto [name, prefix] : packages)
		loadPackage(name, prefix).check();
}

namespace res {
PTexture getTexture(Str name) { return ResManager::instance().getTexture(name, false); }
PTexture getGuiTexture(Str name) { return ResManager::instance().getTexture(format("gui/%.zar", name), false); }
const Font &getFont(Str name) { return ResManager::instance().getFont(name); }
const game::Tile &getTile(Str name) { return ResManager::instance().getTile(name); }
}
