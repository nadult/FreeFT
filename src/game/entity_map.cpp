// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "entity_map.h"
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

	int EntityMap::pixelIntersect(const int2 &pos, FlagsType flags) const {
		return Grid::pixelIntersect(pos, [](const Grid::ObjectDef &object, const int2 &pos)
			{ return ((const Entity*)object.ptr)->testPixel(pos); },  flags);
	}

	void EntityMap::updateOccluderId(int object_id) {
		DASSERT(object_id >= 0 && object_id < size());
		auto &object = (*this)[object_id];
		DASSERT(object.ptr);

		FBox bbox = object.ptr->boundingBox();
		bbox = {bbox.x(), 0.0f, bbox.z(), bbox.ex(), max(0.0f, bbox.y()), bbox.ez()};

		vector<int> temp;
		temp.reserve(128);
		m_tile_map.findAll(temp, bbox);

		int best_occluder_id = -1;
		float best_pos = 0.0f;

		for(int n = 0; n < (int)temp.size(); n++) {
			const auto &object = m_tile_map[temp[n]];
			int occluder_id = object.occluder_id;
			if(occluder_id != -1) {
				if(best_occluder_id == -1 || object.bbox.ey() > best_pos) {
					best_pos = object.bbox.ey();
					best_occluder_id = occluder_id;
				}
			}
		}
				
		object.occluder_id = best_occluder_id;
	}

	int EntityMap::add(Dynamic<Entity> &&ptr, int index) {
		DASSERT(ptr && index >= -1);
		if(index == -1)
			index = findFreeObject();
		Entity *entity = ptr.get();

		Grid::add(index, Grid::ObjectDef(entity, entity->boundingBox(), entity->screenRect(), entity->flags() | Flags::visible));
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

		if(Grid::update(index, Grid::ObjectDef(entity, entity->boundingBox(), entity->screenRect(), entity->flags() | Flags::visible)))
			updateOccluderId(index);
	}

	void EntityMap::loadFromXML(const XmlDocument &doc) {
		//TODO: exception safety when loading...
		auto main_node = doc.child("entity_map");
	
		clear();
		
		if(!main_node) {
			resize(m_tile_map.dimensions());
			return;
		}

		int2 size = main_node.attrib<int2>("size");
		int tile_count = main_node.attrib<int>("entity_count");

		//TODO: duplicated code here and in TileMap
		ASSERT(size.x > 0 && size.y > 0 && size.x <= 16 * 1024 && size.y <= 16 * 1024);
		resize(size);

		auto node = main_node.child();
		while(node) {
			Dynamic<Entity> new_entity(Entity::construct(node));
			add(move(new_entity));
			node = node.sibling();
		}
	}

	void EntityMap::saveToXML(XmlDocument &doc) const {
		auto main_node = doc.addChild("entity_map");
		main_node.addAttrib("size", dimensions());

		vector<int> indices;
		indices.reserve(size());
		for(int n = 0; n < size(); n++)
			if((*this)[n].ptr)
				indices.push_back(n);
		main_node.addAttrib("entity_count", (int)indices.size());

		std::sort(indices.begin(), indices.end(), [this](int a, int b) {
			const float3 p1 = (*this)[a].bbox.min(), p2 = (*this)[b].bbox.min();
			return p1.x == p2.x? p1.y == p2.y? p1.z < p2.z : p1.y < p2.y : p1.x < p2.x;
		} );

		for(int n = 0; n < (int)indices.size(); n++)
			(*this)[indices[n]].ptr->save(main_node);
	}
		
	void EntityMap::updateVisibility(const OccluderConfig &config) {
		config.setVisibilityFlag(*(Grid*)this, Flags::visible);
		updateNodes();
	}

}

