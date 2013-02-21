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

#include "game/tile_map.h"
#include "game/tile.h"
#include "gfx/scene_renderer.h"
#include "sys/profiler.h"
#include "sys/xml.h"
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
				new_map.Grid::add(obj);
		}
		Grid::swap(new_map);
	}

	int TileMap::add(const Tile *tile, const int3 &pos) {
		DASSERT(tile);

		FBox bbox(pos, pos + tile->bboxSize());
		IRect rect = tile->rect() + worldToScreen(pos);
		ASSERT(findAny(bbox) == -1);
		return Grid::add(Grid::ObjectDef((void*)tile, bbox, rect, tileIdToFlag(tile->type())|visibility_flag));
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
		
	int TileMap::pixelIntersect(const int2 &pos, int flags) const {
		return Grid::pixelIntersect(pos,
			[](const Grid::ObjectDef &object, const int2 &pos)
				{ return ((const Tile*)object.ptr)->testPixel(pos - worldToScreen((int3)object.bbox.min)); },
			   flags);
	}

	void TileMap::updateVisibility() {
		//TODO: update only occluders that has changed
		for(int n = 0; n < size(); n++) {
			Object &object = m_objects[n];
			if(object.ptr && object.occluder_id != -1) {
				if(m_occluder_map[object.occluder_id].is_visible)
					object.flags |= visibility_flag;
				else
					object.flags &= ~visibility_flag;
			}
		}

		//TODO: use dirty flags to update nodes lazily?
		updateNodes();
	}

	void TileMap::loadFromXML(const XMLDocument &doc) {
		XMLNode main_node = doc.child("tile_map");
		ASSERT(main_node);

		clear();

		int2 size = main_node.int2Attrib("size");
		int tile_count = main_node.intAttrib("tile_count");

		ASSERT(size.x > 0 && size.y > 0 && size.x <= 16 * 1024 && size.y <= 16 * 1024);
		resize(size);

		XMLNode tnode = main_node.child("tile");
		while(tnode) {
			const Tile *tile = &*Tile::mgr[tnode.attrib("name")];
			XMLNode inode = tnode.child("i");
			while(inode) {
				int3 pos = inode.int3Attrib("pos");
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

			int cmp = strcmp(obj1.ptr->resourceName(), obj2.ptr->resourceName());
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

}
