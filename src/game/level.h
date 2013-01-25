/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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

		void load(const char *file_name);
		void save(const char *file_name) const;

		TileMap tile_map;
		EntityMap entity_map;
	};

}

#endif
