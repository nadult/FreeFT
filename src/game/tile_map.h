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

		void serialize(Serializer&);
		void loadFromXML(const XMLDocument&);
		void saveToXML(XMLDocument&) const;
		void swap(const TileMap&);

		void legacyLoad(Serializer&);

		OccluderMap &occluderMap() { return m_occluder_map; }
		const OccluderMap &occluderMap() const { return m_occluder_map; }
		void updateVisibility();

	protected:
		OccluderMap m_occluder_map;
	};

}


#endif
