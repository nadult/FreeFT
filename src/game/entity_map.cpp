/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "entity_map.h"


namespace game
{

	EntityMap::EntityMap(const int2 &dimensions) :Grid(dimensions) { }
	
	void EntityMap::resize(const int2 &new_size) {
		//TODO: occluders?
		EntityMap new_map(new_size);
		for(int n = 0; n < size(); n++) {
			const Grid::ObjectDef &obj = Grid::operator[](n);
			if(obj.ptr && new_map.isInside(obj.bbox))
				new_map.Grid::add(obj);
		}
		Grid::swap(new_map);
	}

	int EntityMap::pixelIntersect(const int2 &pos, int flags) const {
		return Grid::pixelIntersect(pos, [](const Grid::ObjectDef &object, const int2 &pos)
			{ return ((const Entity*)object.ptr)->testPixel(pos); },  flags);
	}
		
	void EntityMap::add(Entity *entity) {
		DASSERT(entity);

		int id = Grid::add(Grid::ObjectDef(entity, entity->boundingBox(), entity->screenRect(),
					entity->colliderType() | visibility_flag));
		entity->m_grid_index = id;
	}

	void EntityMap::remove(const Entity *entity) {
		DASSERT(entity);
		DASSERT(entity->m_grid_index != -1);
		Grid::remove(entity->m_grid_index);
		entity->m_grid_index = -1;
	}

	void EntityMap::update(const Entity *entity) {
		DASSERT(entity);
		DASSERT(entity->m_grid_index != -1);
		Grid::update(entity->m_grid_index,
				Grid::ObjectDef(const_cast<Entity*>(entity), entity->boundingBox(), entity->screenRect(),
					entity->colliderType() | visibility_flag));
	}

}

