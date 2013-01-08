#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "base.h"
#include "grid.h"

namespace gfx { class Tile; class SceneRenderer; };

class TileMap: public Grid {
public:
	struct TileDef {
		const gfx::Tile *ptr;
		FBox bbox;
		IRect rect;
		int flags;
	};

	const TileDef &operator [](int idx) const
		{ return reinterpret_cast<const TileDef&>(Grid::operator[](idx)); }

	TileMap(const int2 &dimensions = int2(0, 0)) :Grid(dimensions) { }

	int add(const gfx::Tile*, const int3 &pos);
	void resize(const int2 &new_dims);

	void serialize(Serializer&);
	void loadFromXML(const XMLDocument&);
	void saveToXML(XMLDocument&) const;
	void swap(const TileMap&);

	void legacyLoad(Serializer&);
};


#endif
