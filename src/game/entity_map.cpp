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
				new_map.Grid::add(n, obj);
		}
		Grid::swap(new_map);
	}

	int EntityMap::pixelIntersect(const int2 &pos, int flags) const {
		return Grid::pixelIntersect(pos, [](const Grid::ObjectDef &object, const int2 &pos)
			{ return ((const Entity*)object.ptr)->testPixel(pos); },  flags);
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

	int EntityMap::add(unique_ptr<Entity> &&ptr, int index) {
		DASSERT(ptr && index >= -1);
		if(index == -1)
			index = findFreeObject();
		Entity *entity = ptr.get();

		Grid::add(index, Grid::ObjectDef(entity, entity->boundingBox(), entity->screenRect(), entity->colliderType()));
		updateOccluderId(index);
		ptr.release();

		return index;
	}

	void EntityMap::remove(int index) {
		DASSERT(index >= 0 && index < size());
		Grid::remove(index);
		delete (*this)[index].ptr;
	}

	void EntityMap::update(int index) {
		DASSERT(index >= 0 && index < size());
		ObjectDef &object = (*this)[index];
		Entity *entity = object.ptr;
		DASSERT(entity);

		if(Grid::update(index, Grid::ObjectDef(entity, entity->boundingBox(), entity->screenRect(), object.flags)))
			updateOccluderId(index);
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
			unique_ptr<Entity> new_entity(Entity::construct(node));
			add(std::move(new_entity));
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

