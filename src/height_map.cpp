#include "height_map.h"
#include <cstring>
#include "gfx/texture.h"

HeightMap::HeightMap(int2 size) :m_size(0, 0) {
	resize(size);
}

void HeightMap::resize(int2 size) {
	int x_nodes = (size.x + node_size - 1) / node_size;
	int y_nodes = (size.y + node_size - 1) / node_size;
	m_size = int2(x_nodes, y_nodes);

	int node_count = x_nodes * y_nodes;
	m_nodes.resize(node_count);
	m_dnodes.clear();
	m_node_indices.clear();

	for(int n = 0; n < node_count; n++) {
		Node &node = m_nodes[n];
		node.m_dnode_idx = -1;
		node.m_min_y = node.m_max_y = 0;
	}
}

void HeightMap::update(const TileMap &tile_map) {
	m_dnodes.clear();
	m_node_indices.clear();

	for(int n = 0; n < (int)m_nodes.size(); n++) {
		Node &node = m_nodes[n];
		int3 node_pos = int3(n % m_size.x, 0, n / m_size.x) * node_size;
		
		DetailNode dnode;
		memset(dnode.m_data, 0, sizeof(dnode.m_data));

		//FIXME: im assuming here that there are no bigger tiles than TileMapNode
		IBox dbox(node_pos, node_pos + int3(node_size, 256, node_size));

		int tnode_x = Max(0, node_pos.x / TileMapNode::size_x - 1);
		int tnode_z = Max(0, node_pos.z / TileMapNode::size_z - 1);

		int end_x = Min(tile_map.nodeCountX(), tnode_x + 3);
		int end_z = Min(tile_map.nodeCountZ(), tnode_z + 3);

		for(int tx = tnode_x; tx < end_x; tx++)
			for(int tz = tnode_z; tz < end_z; tz++) {
				const TileMapNode &node = tile_map(int2(tx, tz));
				int3 tnode_pos(tx * TileMapNode::size_x, 0, tz * TileMapNode::size_z);
				
				for(int i = 0; i < node.instanceCount(); i++) {
					const TileInstance &inst = node(i);
					IBox bbox = inst.boundingBox() + tnode_pos;
					
					if(Overlaps(dbox, bbox)) {
						int3 pos = bbox.min - dbox.min;
						int3 size = bbox.Size();

						int w = Min(node_size - pos.x, size.x);
						int h = Min(node_size - pos.z, size.z);

						u8 *ptr = dnode.m_data + pos.x + pos.z * node_size;
						for(int y = 0; y < h; y++)
							for(int x = 0; x < w; x++)
								ptr[x + y * node_size] = (u8)(pos.y + size.y);
					}
				}
			}

		u8 min = dnode.m_data[0], max = dnode.m_data[0];
		for(int i = 1; i < node_size * node_size; i++) {
			min = Min(min, dnode.m_data[i]);
			max = Max(max, dnode.m_data[i]);
		}

		node.m_min_y = min;
		node.m_max_y = max;

		if(node.m_min_y != node.m_max_y) {
			node.m_dnode_idx = (int)m_dnodes.size();
			m_dnodes.push_back(dnode);
	//		m_node_indices.push_back(n);
		}
		else {
			node.m_dnode_idx = -1;
		}
	}	
}


gfx::PTexture HeightMap::getTexture() const {
	gfx::Texture tex(m_size.x * node_size, m_size.y * node_size);

	for(int n = 0; n < (int)m_nodes.size(); n++) {
		int node_x = (n % m_size.x) * node_size, node_y = (n / m_size.x) * node_size;
		const Node &node = m_nodes[n];
		const DetailNode *dnode = node.m_dnode_idx == -1? nullptr : &m_dnodes[node.m_dnode_idx];

		if(dnode) {
			for(int y = 0; y < node_size; y++)
				for(int x = 0; x < node_size; x++) {
					int height = dnode->m_data[x + y * node_size];
					tex(x + node_x, y + node_y) = Color(Min(height * 30, 255), height? 255 : 0, height? 255 : 0);
				}
		}
		else {
			int height = node.m_min_y;
			Color col(Min(height * 30, 255), height? 255 : 0, height? 255 : 0);

			for(int y = 0; y < node_size; y++)
				for(int x = 0; x < node_size; x++)
					tex(x + node_x, y + node_y) = col;
		}
	}
	
	gfx::PTexture out = new gfx::DTexture;
	out->SetSurface(tex);
	return out;
}

void HeightMap::printInfo() const {
	printf("HeightMap(%d, %d): %d x %d nodes (%dx%d in size)\n",
			m_size.x * node_size, m_size.y * node_size, m_size.x, m_size.y, node_size, node_size);
	printf("%d Nodes * %d bytes = %.0f KB\n", (int)m_nodes.size(), (int)sizeof(Node),
			double(m_nodes.size() * sizeof(Node)) / double(1024));
	printf("%d DetailNodes * %d bytes = %.0f KB\n", (int)m_dnodes.size(), (int)sizeof(DetailNode),
			double(m_dnodes.size() * sizeof(DetailNode)) / double(1024));

}

