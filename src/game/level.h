// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/tile_map.h"
#include "game/entity_map.h"
#include "occluder_map.h"

namespace game
{
	class Level
	{
	public:
		Level();

		// Loads maps from .xml and .mod files; .mod maps are essentially
		// ed scripts that are applied to .xml map with the same base name
		// Example map name: demo_map.xml
		// file data/maps/demo_map.xml will be loaded
		void load(string map_name);
		void save(string map_name) const;

		TileMap tile_map;
		EntityMap entity_map;
	};

}
