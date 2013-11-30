/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_LEVEL_H
#define GAME_LEVEL_H

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
		void load(const char *file_name);
		void save(const char *file_name) const;

		TileMap tile_map;
		EntityMap entity_map;
	};

}

#endif
