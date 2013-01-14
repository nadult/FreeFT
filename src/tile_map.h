#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "base.h"
#include "grid.h"
#include "occluder_map.h"

namespace gfx { class Tile; class SceneRenderer; };

class TileMap: public Grid {
public:
	typedef Grid::TObjectDef<gfx::Tile> TileDef;

	const TileDef &operator [](int idx) const
		{ return reinterpret_cast<const TileDef&>(Grid::operator[](idx)); }

	TileMap(const int2 &dimensions = int2(0, 0));

	int add(const gfx::Tile*, const int3 &pos);
	void resize(const int2 &new_dims);

	void serialize(Serializer&);
	void loadFromXML(const XMLDocument&);
	void saveToXML(XMLDocument&) const;
	void swap(const TileMap&);

	void legacyLoad(Serializer&);

	OccluderMap &occluderMap() { return m_occluder_map; }
	const OccluderMap &occluderMap() const { return m_occluder_map; }

protected:
	OccluderMap m_occluder_map;
};


#endif
