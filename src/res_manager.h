// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"

DEFINE_ENUM(ResType, tile, sprite, font, texture, other);

// Only single instance allowed
class ResManager {
  public:
	ResManager(VDeviceRef, bool console_mode = false);
	~ResManager();

	ResManager(const ResManager &) = delete;
	void operator=(ResManager &) = delete;

	static ResManager &instance() {
		DASSERT(g_instance);
		return *g_instance;
	}

	Ex<PVImageView> getTexture(Str, bool font_tex);
	const Font &getFont(Str);
	const game::Tile &getTile(Str);
	const std::map<string, Dynamic<game::Tile>> &allTiles();

	// Other resource will only be stored in manager with loadResource.
	// Otherwise it will simply be loaded from file every time when getOther() is called.
	vector<char> getOther(Str) const;

	Ex<void> loadResource(Str name, Stream &, ResType);
	Ex<void> loadResource(Str name, CSpan<char> data, ResType);

	Maybe<ResType> classifyPath(Str, bool ignore_prefix = false) const;
	// Will return none if path doesn't match resource prefix & suffix
	Maybe<string> resourceName(Str path, ResType) const;

	string fullPath(Str res_name, ResType) const;
	const auto &dataPath() const { return m_data_path; }

	Pair<Str> prefixSuffix(ResType type) const { return m_paths[type]; }

  private:
	Ex<void> loadPackage(Str, Str);

	static ResManager *g_instance;
	struct Impl;
	Dynamic<Impl> m_impl;

	VDeviceRef m_device;
	EnumMap<ResType, Pair<string, string>> m_paths;
	string m_data_path;
	bool m_console_mode;
};
