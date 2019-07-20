// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

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

		void remove(int idx);
		void update(int idx);
		
		int pixelIntersect(const int2 &pos, FlagsType flags = Flags::all) const;

		void loadFromXML(const XmlDocument&);
		void saveToXML(XmlDocument&) const;
		void swap(const TileMap&);

		void legacyConvert(Stream &in, Stream &out);

		OccluderMap &occluderMap() { return m_occluder_map; }
		const OccluderMap &occluderMap() const { return m_occluder_map; }
		void updateVisibility(const OccluderConfig&);

		int add(const Tile*, const int3 &pos);
		int maybeAdd(const Tile&, const int3&);

	private:
		OccluderMap m_occluder_map;
	};

}
