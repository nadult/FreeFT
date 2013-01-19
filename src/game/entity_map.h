/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
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

	class EntityMap: public Grid
	{
	public:
		typedef Grid::TObjectDef<Entity> ObjectDef;
	
		const ObjectDef &operator [](int idx) const
			{ return reinterpret_cast<const ObjectDef&>(Grid::operator[](idx)); }

		explicit EntityMap(const TileMap&, const int2 &dimensions = int2(0, 0));
		void resize(const int2 &new_dims);
	
		void add(Entity*);
		void remove(const Entity*);
		void update(const Entity*);
		
		int pixelIntersect(const int2 &pos, int flags = collider_flags) const;
		void updateVisibility();

	protected:
		const TileMap &m_tile_map;
	};

}

#endif
