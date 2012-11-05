#ifndef HEIGHT_MAP_H
#define HEIGHT_MAP_H

#include "tile_map.h"


class HeightMap {
public:
	HeightMap(int2 size);

	void update(const TileMap&);

	enum {
		nodeSizeX = 6,
		nodeSizeZ = 6,
	};

	struct Node {
		int m_dnode_idx;
		u16 temp;
		u8 m_min_y, u_max_y;
	};

	struct DetailNode {
		u8 m_height[nodeSizeX][nodeSizeZ];
		int m_node_idx;
	};

protected:
	vector<Node> m_nodes;
	vector<DetailNode> m_dnodes;
	int2 m_size;
};


#endif
