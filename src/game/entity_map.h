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

#ifndef ENTITY_MAP_H
#define ENTITY_MAP_H

#include "grid.h"
#include "occluder_map.h"
#include "game/entity.h"
#include "game/tile_map.h"

namespace game
{

	// Grid based container for entities
	// when removing entities, they will be automaticly deleted
	class EntityMap: public Grid
	{
	public:
		typedef Grid::TObjectDef<Entity> ObjectDef;

		explicit EntityMap(const TileMap&, const int2 &dimensions = int2(0, 0));
		~EntityMap();
		EntityMap(const EntityMap&) = delete;
		void operator=(const EntityMap&) = delete;
	
		const ObjectDef &operator [](int idx) const
			{ return reinterpret_cast<const ObjectDef&>(Grid::operator[](idx)); }
		ObjectDef &operator[](int idx)
			{ return reinterpret_cast<ObjectDef&>(Grid::operator[](idx)); }

		void resize(const int2 &new_dims);
	
		void add(Entity*);

		void update(Entity*);
		void update(int entity_id);

		void remove(Entity*);
		void remove(int entity_id);
		
		int pixelIntersect(const int2 &pos, int flags = collider_flags) const;
		void updateVisibility();

	protected:
		void updateOccluderId(int object_id);

		const TileMap &m_tile_map;
	};

}

#endif
