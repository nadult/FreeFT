/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "entity_map.h"
#include "sys/xml.h"
#include <algorithm>

namespace game
{

	EntityMap::~EntityMap() {
		for(int n = 0; n < size(); n++)
			delete (*this)[n].ptr;
	}

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
		updateOccluderId(id);
	}

	void EntityMap::updateOccluderId(int object_id) {
		DASSERT(object_id >= 0 && object_id < size());
		auto &object = (*this)[object_id];
		DASSERT(object.ptr);

		FBox bbox = object.ptr->boundingBox();
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
				
		object.occluder_id = best_occluder_id;
	}

	void EntityMap::remove(Entity *entity) {
		DASSERT(entity && entity->m_grid_index != -1);
		remove(entity->m_grid_index);
	}
	
	void EntityMap::remove(int entity_id) {
		DASSERT(entity_id >= 0 && entity_id < size());
		Grid::remove(entity_id);
		delete (*this)[entity_id].ptr;
	}

	void EntityMap::update(Entity *entity) {
		DASSERT(entity && entity->m_grid_index != -1);
		update(entity->m_grid_index);
	}

	void EntityMap::update(int entity_id) {
		auto &object = (*this)[entity_id];
		FBox old_bbox = object.bbox;
		Entity *entity = object.ptr;

		Grid::update(entity_id,
				Grid::ObjectDef(entity, entity->boundingBox(), entity->screenRect(), entity->colliderType() | visibility_flag));
		
		if(object.bbox != old_bbox)
			updateOccluderId(entity_id);
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

	void EntityMap::loadFromXML(const XMLDocument &doc) {
		//TODO: exception safety when loading...
		XMLNode main_node = doc.child("entity_map");
	
		clear();
		
		if(!main_node) {
			resize(m_tile_map.dimensions());
			return;
		}

		int2 size = main_node.int2Attrib("size");
		int tile_count = main_node.intAttrib("entity_count");

		//TODO: duplicated code here and in TileMap
		ASSERT(size.x > 0 && size.y > 0 && size.x <= 16 * 1024 && size.y <= 16 * 1024);
		resize(size);

		XMLNode node = main_node.child();
		while(node) {
			Entity *entity = Entity::construct(node);
			add(entity);
			node = node.sibling();
		}
	}

	void EntityMap::saveToXML(XMLDocument &doc) const {
		XMLNode main_node = doc.addChild("entity_map");
		main_node.addAttrib("size", dimensions());

		std::vector<int> indices;
		indices.reserve(size());
		for(int n = 0; n < size(); n++)
			if((*this)[n].ptr)
				indices.push_back(n);
		main_node.addAttrib("entity_count", (int)indices.size());

		std::sort(indices.begin(), indices.end(), [this](int a, int b) {
			const float3 p1 = (*this)[a].bbox.min, p2 = (*this)[b].bbox.min;
			return p1.x == p2.x? p1.y == p2.y? p1.z < p2.z : p1.y < p2.y : p1.x < p2.x;
		} );

		for(int n = 0; n < (int)indices.size(); n++)
			(*this)[indices[n]].ptr->save(main_node);
	}
}

