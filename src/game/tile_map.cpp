/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/tile_map.h"
#include "game/tile.h"
#include "gfx/scene_renderer.h"
#include <algorithm>


namespace game {

	TileMap::TileMap(const int2 &dimensions)
		:Grid(dimensions), m_occluder_map(*this) { }

	void TileMap::resize(const int2 &new_size) {
		//TODO: occluders?
		TileMap new_map(new_size);
		for(int n = 0; n < size(); n++) {
			const Grid::ObjectDef &obj = Grid::operator[](n);
			if(obj.ptr && new_map.isInside(obj.bbox))
				new_map.Grid::add(new_map.findFreeObject(), obj);
		}
		Grid::swap(new_map);
	}

	int TileMap::add(const Tile *tile, const int3 &pos) {
		DASSERT(tile);

		FBox bbox(pos, pos + tile->bboxSize());
		IRect rect = tile->rect() + worldToScreen(pos);
		ASSERT(findAny(bbox) == -1);
		int index = findFreeObject();
		Grid::add(index, Grid::ObjectDef((void*)tile, bbox, rect, tile->flags() | Flags::visible));
		return index;
	}

	void TileMap::remove(int idx) {
		DASSERT(idx >= 0 && idx < size());

		//TODO: speed up somehow?
		if((*this)[idx].ptr) {
			int occluder_id = (*this)[idx].occluder_id;
			if(occluder_id != -1) {
				OccluderMap::Occluder &occluder = m_occluder_map[occluder_id];
				for(int n = 0; n < (int)occluder.objects.size(); n++)
					if(occluder.objects[n] == idx) {
						occluder.objects[n] = occluder.objects.back();
						occluder.objects.pop_back();
					}
			}
		}
		Grid::remove(idx);
	}

	void TileMap::update(int idx) {
		THROW("WRITE ME");
	}
		
	int TileMap::pixelIntersect(const int2 &pos, Flags::Type flags) const {
		return Grid::pixelIntersect(pos,
			[](const Grid::ObjectDef &object, const int2 &pos)
				{ return ((const Tile*)object.ptr)->testPixel(pos - worldToScreen((int3)object.bbox.min)); },
			   flags);
	}

	void TileMap::loadFromXML(const XMLDocument &doc) {
		XMLNode main_node = doc.child("tile_map");
		ASSERT(main_node);

		clear();

		int2 size = main_node.attrib<int2>("size");
		int tile_count = main_node.attrib<int>("tile_count");

		ASSERT(size.x > 0 && size.y > 0 && size.x <= 16 * 1024 && size.y <= 16 * 1024);
		resize(size);

		XMLNode tnode = main_node.child("tile");
		while(tnode) {
			const Tile *tile = res::tiles()[tnode.attrib("name")].get();
			XMLNode inode = tnode.child("i");
			while(inode) {
				int3 pos = inode.attrib<int3>("pos");
				add(tile, pos);
				inode = inode.sibling("i");
			}
			tnode = tnode.sibling("tile");
		}

		m_occluder_map.loadFromXML(doc);
	}

	void TileMap::saveToXML(XMLDocument &doc) const {
		XMLNode main_node = doc.addChild("tile_map");
		main_node.addAttrib("size", dimensions());

		std::vector<int> indices;
		indices.reserve(size());
		for(int n = 0; n < size(); n++)
			if((*this)[n].ptr)
				indices.push_back(n);
		main_node.addAttrib("tile_count", (int)indices.size());

		std::sort(indices.begin(), indices.end(), [this](int a, int b) {
			const auto &obj1 = (*this)[a];
			const auto &obj2 = (*this)[b];

			int cmp = strcmp(obj1.ptr->resourceName().c_str(), obj2.ptr->resourceName().c_str());
			if(cmp == 0) {
				const float3 p1 = obj1.bbox.min, p2 = obj2.bbox.min;
				return p1.x == p2.x? p1.y == p2.y? p1.z < p2.z : p1.y < p2.y : p1.x < p2.x;
			}

			return cmp < 0;
		} );
		
		const Tile *prev = nullptr;
		XMLNode tile_node;

		PodArray<int> tile_ids(size());
		for(int n = 0; n < size(); n++)
			tile_ids[n] = -1;
		int object_id = 0;

		for(int n = 0; n < (int)indices.size(); n++) {
			const ObjectDef &object = (*this)[indices[n]];
			tile_ids[indices[n]] = object_id++;

			if(object.ptr != prev) {
				tile_node = main_node.addChild("tile");
				tile_node.addAttrib("name", doc.own(object.ptr->resourceName()));
				prev = object.ptr;
			}

			XMLNode instance = tile_node.addChild("i");
			instance.addAttrib("pos", int3(object.bbox.min));
		}

		m_occluder_map.saveToXML(tile_ids, doc);
	}
		
	void TileMap::updateVisibility(const OccluderConfig &config) {
		config.setVisibilityFlag(*(Grid*)this, Flags::visible);
		updateNodes();
	}

}
