#ifndef TILE_SELECTOR_H
#define TILE_SELECTOR_H

#include "base.h"
#include "gfx/tile.h"

class TileGroup;

class TileSelector {
public:
	TileSelector(int2 res);

	void loop();
	void draw();

	int tileId() const { return m_tile_id; }

	void setSource(const vector<gfx::Tile> *tiles) {
		m_tiles = tiles;
		m_tile_group = nullptr;
	}
	void setSource(const TileGroup* tile_group) {
		m_tiles = nullptr;
		m_tile_group = tile_group;
	}
	int tileCount() const {
		return m_tiles? (int)m_tiles->size() : 0;
	}
	const gfx::Tile *getTile(int idx) const {
		return m_tiles? &(*m_tiles)[idx] : nullptr;
	}

protected:
	const vector<gfx::Tile> *m_tiles;
	const TileGroup *m_tile_group;

	IRect m_view;
	int m_offset;
	int m_tile_id;
};


#endif
