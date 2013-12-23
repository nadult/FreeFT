/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "base.h"
#include "grid.h"
#include "occluder_map.h"

namespace game {

	class TileMap: public Grid {
	public:
		typedef Grid::TObjectDef<const Tile> ObjectDef;

		const ObjectDef &operator [](int idx) const
			{ return reinterpret_cast<const ObjectDef&>(Grid::operator[](idx)); }

		explicit TileMap(const int2 &dimensions = int2(0, 0));
		void resize(const int2 &new_dims);

		int add(const Tile*, const int3 &pos);
		void remove(int idx);
		void update(int idx);
		
		int pixelIntersect(const int2 &pos, int flags = collider_flags) const;

		void loadFromXML(const XMLDocument&);
		void saveToXML(XMLDocument&) const;
		void swap(const TileMap&);

		void legacyConvert(Stream &in, Stream &out);

		OccluderMap &occluderMap() { return m_occluder_map; }
		const OccluderMap &occluderMap() const { return m_occluder_map; }
		void updateVisibility();

	protected:
		OccluderMap m_occluder_map;
	};

}


#endif
