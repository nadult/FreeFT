#ifndef HEIGHT_MAP_H
#define HEIGHT_MAP_H

#include "tile_map.h"
#include "gfx/device.h"

class HeightMap {
public:
	HeightMap(int2 size);

	void update(const TileMap&);

	enum {
		node_size = 8,
	};

	struct Node {
		int m_dnode_idx;
		u16 temp;
		u8 m_min_y, m_max_y;
	};

	struct DetailNode {
		u8 m_data[node_size * node_size];
	};

	void resize(int2 size);
	int2 size() const { return m_size * node_size; }

	gfx::PTexture getTexture() const;
	void printInfo() const;

protected:
	vector<Node> m_nodes;
	vector<DetailNode> m_dnodes;
	vector<int> m_node_indices; // dnode idx -> node idx
	int2 m_size; // node count in x * node count in z
};


#endif
