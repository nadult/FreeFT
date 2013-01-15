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

	//TODO: this is stupid
	EntityMap::EntityMap(const TileMap &tile_map, const int2 &dimensions)
		:Grid(dimensions), m_tile_map(tile_map) { }
	
	void EntityMap::resize(const int2 &new_size) {
		EntityMap new_map(m_tile_map, new_size);
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

		FBox bbox = entity->boundingBox();
		bbox.max.y = bbox.min.y;
		bbox.min.y = 0.0f;

		vector<int> temp;
		temp.reserve(128);
		m_tile_map.findAll(temp, bbox);

		int best_occluder_id = -1;
		float best_pos = 0.0f;

		for(int n = 0; n < (int)temp.size(); n++) {
			const auto &object = m_tile_map[temp[n]];
			int occluder_id = object.occluder_id;
			if(occluder_id != -1) {
				if(best_occluder_id == -1 || object.bbox.max.y > best_pos) {
					best_pos = object.bbox.max.y;
					best_occluder_id = occluder_id;
				}
			}
		}
				
		Grid::operator[](id).occluder_id = best_occluder_id;
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

	void EntityMap::updateVisibility() {
		const OccluderMap &occmap = m_tile_map.occluderMap();

		for(int n = 0; n < size(); n++) {
			auto &object = Grid::operator[](n);
			if(object.occluder_id != -1) {
				bool is_visible = occmap[object.occluder_id].is_visible;
				if(is_visible)
					object.flags |= visibility_flag;
				else
					object.flags &= ~visibility_flag;
			}
		}
	}

}

