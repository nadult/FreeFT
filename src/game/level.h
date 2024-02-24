// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/entity_map.h"
#include "game/tile_map.h"
#include "occluder_map.h"

namespace game {
class Level {
  public:
	Level();

	// Loads maps from .xml and .mod files; .mod maps are essentially
	// ed scripts that are applied to .xml map with the same base name
	// map name demo_map.xml -> file data/maps/demo_map.xml
	Ex<void> load(ZStr map_name);
	Ex<void> save(ZStr map_name) const;

	TileMap tile_map;
	EntityMap entity_map;
};

}
