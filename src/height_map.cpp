#include "height_map.h"

HeightMap::HeightMap(int2 size) :m_size(size) {
}

void HeightMap::update(const TileMap &tile_map) {
	DAssert(m_size == tile_map.size());


}
