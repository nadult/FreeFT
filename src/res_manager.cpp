// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "res_manager.h"

#include "game/tile.h"
#include <fwk/gfx/font.h>
#include <fwk/gfx/texture.h>
#include <fwk/io/file_stream.h>
#include <fwk/io/file_system.h>
#include <fwk/io/memory_stream.h>
#include <fwk/io/url_fetch.h>
#include <fwk/io/gzip_stream.h>
#include <fwk/gfx/gl_texture.h>
#include <fwk/hash_map.h>
#include <fwk/io/package_file.h>
#include <map>

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

struct ResManager::Impl {
	FwdMember<HashMap<string, PTexture>>  textures;
	std::map<string, Dynamic<game::Tile>> tiles;
	std::map<string, Font> fonts;
	std::map<string, vector<char>> others;
};

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

	m_impl.emplace();
	double time = getTime();
	if(!m_console_mode)
		preloadEmbedded();
	time = getTime() - time;
	printf("Loaded embedded resources in: %.2f msec\n", time * 1000.0);
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
	auto &textures = m_impl->textures;
	auto it = textures.find(name);
	if(it == textures.end()) {
		auto tex = Texture::load(fullPath(name, ResType::texture));
		tex.check();
		if(font_tex)
			fixGrayTransTexture(*tex);
		auto gl_tex = GlTexture::make(*tex);
		textures.emplace(name, gl_tex);
		return gl_tex;
	}
	return it->value;
}

const Font &ResManager::getFont(Str name) {
	auto &fonts = m_impl->fonts;
	auto it = fonts.find(name);
	if(it == fonts.end()) {
		auto vcore = FontCore::load(fullPath(name, ResType::font));
		vcore.check();
		FontCore core(move(*vcore));
		auto tex = getTexture("fonts/" + core.textureName(), true);
		it = fonts.emplace(name, Font(move(core), move(tex))).first;
	}
	return it->second;
}

const game::Tile &ResManager::getTile(Str name) {
	auto &tiles = m_impl->tiles;
	auto it = tiles.find(name);
	if(it == tiles.end()) {
		auto ldr = fileLoader(fullPath(name, ResType::tile));
		ldr.check();
		Dynamic<game::Tile> tile;
		tile.emplace();
		auto result = tile->load(*ldr);
		tile->setResourceName(name);
		result.check();
		it = tiles.emplace(name, move(tile)).first;
	}
	return *it->second;
}
	
vector<char> ResManager::getOther(Str name)  const{
	auto it = m_impl->others.find(name);
	if(it == m_impl->others.end()) {
		auto data = loadFile(format("%%", m_data_path, name));
		data.check();
		return move(*data);
	}
	return it->second;
}
	
const std::map<string, Dynamic<game::Tile>> &ResManager::allTiles() { return m_impl->tiles; }
	
Ex<void> ResManager::loadResource(Str name, Stream &sr, ResType type) {
	DASSERT(sr.isLoading());

	if(type == ResType::tile) {
		Dynamic<game::Tile> tile;
		tile.emplace();
		EXPECT(tile->load(sr));
		tile->setResourceName(name);
		m_impl->tiles[name] = move(tile);
	}
	else if(type == ResType::sprite) {
		FATAL("write me");
	}
	else if(type == ResType::font) {
		auto doc = EX_PASS(XmlDocument::load(sr));
		auto core = EX_PASS(FontCore::load(doc));
		auto tex = getTexture(format("fonts/%", core.textureName()), true);
		m_impl->fonts.erase(name);
		m_impl->fonts.emplace(name, Font(move(core), move(tex)));
	}
	else if(type == ResType::texture) {
		auto ext = fileExtension(name);
		if(!ext)
			return ERROR("Texture without extension: '%'", name);
		auto tex = EX_PASS(Texture::load(sr, *ext));
		m_impl->textures[name] = GlTexture::make(tex);
	}
	else if(type == ResType::other) {
		vector<char> data(sr.size());
		sr.loadData(data);
		EX_CATCH();
		m_impl->others[name] = move(data);
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
	{prefix #name ".png", cspan(name##_png, name##_png_len).template reinterpret<char>(), fix_trans}
	struct Tex {
		Str name;
		CSpan<char> data;
		bool fix_trans;
	};

	Tex textures[] = {
		TEX("fonts/", liberation_16_0, true),
		TEX("fonts/", liberation_24_0, true),
		TEX("fonts/", liberation_32_0, true),
		TEX("fonts/", liberation_48_0, true),
		TEX("fonts/", transformers_20_0, true),
		TEX("fonts/", transformers_30_0, true),
		TEX("fonts/", transformers_48_0, true),
		TEX("", loading_bar, true),
		TEX("", icons, true)
	};
#undef TEX

	for(auto [name, data, fix] : textures) {
		auto ldr = memoryLoader(data);
		auto tex = Texture::load(ldr, TextureFileType::png);
		tex.check();
		if(fix)
			fixGrayTransTexture(*tex);
		m_impl->textures[name] = GlTexture::make(*tex);
	}

	{
		auto fonts_gz_data = CSpan<u8>(fonts_pack_gz, fonts_pack_gz_len).reinterpret<char>();
		auto fonts_data = gzipDecompress(fonts_gz_data).get();
		auto loader = memoryLoader(fonts_data);

		auto pkg_file = PackageFile::load(loader).get();
		for(int n : intRange(pkg_file.size())) {
			auto font_loader = memoryLoader(pkg_file.data(n));
			string name = pkg_file[n].name;
			removeSuffix(name, ".fnt");
			loadResource(name, font_loader, ResType::font).check();
		}
	}
}

static const Pair<const char*> preload_packages[] = {
	{"data", "data/"},
	{"gui", "data/gui/"},
	{"sprites", "data/sprites/"},
	{"tiles", "data/tiles/"},
};

void ResManager::beginPreloading() {
}

bool ResManager::preloadingStep() {
	return true;
}

namespace res {
PTexture getTexture(Str name, bool fix_trans) {
	return ResManager::instance().getTexture(name, fix_trans);
}

PTexture getGuiTexture(Str name, bool fix_trans) {
	return ResManager::instance().getTexture(format("gui/%.zar", name), fix_trans);
}

const Font &getFont(Str name) { return ResManager::instance().getFont(name); }
const game::Tile &getTile(Str name) { return ResManager::instance().getTile(name); }
}
