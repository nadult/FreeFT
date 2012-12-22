#include "tile_map.h"
#include "gfx/tile.h"
#include "gfx/scene_renderer.h"
#include "sys/profiler.h"
#include "sys/xml.h"
#include <algorithm>

TileInstance::TileInstance(const gfx::Tile *tile, const int3 &pos)
	:m_flags(0), m_tile(tile) {
	DASSERT(m_tile);
	setPos(pos);
}


int3 TileInstance::pos() const {
	return int3(int(m_xz) & 15, int(m_y), int(m_xz) >> 4);
}

IBox TileInstance::boundingBox() const {
	int3 pos = this->pos();
	return IBox(pos, pos + m_tile->bboxSize());
}

IRect TileInstance::screenRect() const {
	return m_tile->rect() + worldToScreen(pos());
}

void TileInstance::setPos(int3 pos) {
	DASSERT(pos.x < TileMapNode::size_x && pos.y < TileMapNode::size_y && pos.z < TileMapNode::size_z);
	DASSERT(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
	m_xz = u8(pos.x | (pos.z << 4));
	m_y  = u8(pos.y);
}

bool TileMapNode::isColliding(const IBox &box) const {
	if(!m_instances.size() || !areOverlapping(box, m_bounding_box))
		return false;

	for(uint i = 0; i < m_instances.size(); i++) {
		const TileInstance &instance = m_instances[i];
		DASSERT(instance.m_tile);

		int3 tilePos = instance.pos();
		IBox tileBox(tilePos, tilePos + instance.m_tile->bboxSize());

		if(areOverlapping(tileBox, box))
			return true;
	}

	return false;
}

bool TileMapNode::isInside(const int3 &point) const {
	return IBox(0, 0, 0, size_x, size_y, size_z).isInside(point);
}
	
const TileInstance *TileMapNode::at(int3 pos) const {
	for(int n = 0; n < (int)m_instances.size(); n++)
		if(m_instances[n].pos() == pos)
			return &m_instances[n];
	return nullptr;
}

void TileMapNode::addTile(const gfx::Tile &tile, int3 pos, bool test_for_collision) {
	if(test_for_collision && isColliding(IBox(pos, pos + tile.bboxSize())))
		return;
	
	DASSERT(isInside(pos));

	TileInstance inst(&tile, pos);
	m_instances.push_back(inst);

	std::sort(m_instances.begin(), m_instances.end());

	m_screen_rect = sum(m_screen_rect, inst.screenRect());
	m_bounding_box = sum(m_bounding_box, inst.boundingBox());
}

void TileMapNode::select(const IBox &box, SelectionMode::Type mode) {
	if(!m_instances.size())
		return;
		
	if(mode == SelectionMode::normal) {
		for(uint i = 0; i < m_instances.size(); i++) {
			TileInstance &instance = m_instances[i];
			instance.select(areOverlapping(box, instance.boundingBox()));
		}
	}
	else if(mode == SelectionMode::add) {
		for(uint i = 0; i < m_instances.size(); i++) {
			TileInstance &instance = m_instances[i];
			bool are_overlapping = areOverlapping(box, instance.boundingBox());
			instance.select(are_overlapping || instance.isSelected());
		}
	}
	else if(mode == SelectionMode::subtract && m_any_selected) {
		for(uint i = 0; i < m_instances.size(); i++) {
			TileInstance &instance = m_instances[i];
			bool are_overlapping = areOverlapping(box, instance.boundingBox());
			instance.select(!are_overlapping && instance.isSelected());
		}
	}
	
	m_any_selected = false;
	for(uint i = 0; i < m_instances.size(); i++)
		m_any_selected |= m_instances[i].isSelected();
}

void TileMapNode::deleteSelected() {
	if(!m_any_selected)
		return;

	for(uint i = 0; i < m_instances.size(); i++)
		if(m_instances[i].isSelected()) {
			m_instances[i--] = m_instances.back();
			m_instances.pop_back();
		}

	if(m_instances.size()) {
		m_screen_rect = m_instances[0].screenRect();
		m_bounding_box = m_instances[0].boundingBox();

		for(uint n = 1; n < m_instances.size(); n++) {
			m_screen_rect = sum(m_screen_rect, m_instances[n].screenRect());
			m_bounding_box = sum(m_bounding_box, m_instances[n].boundingBox());
		}

		sort(m_instances.begin(), m_instances.end());
	}
	else {
		m_screen_rect = IRect(0, 0, 0, 0);
		m_bounding_box = IBox(0, 0, 0, 0, 0, 0);
	}
	
	m_any_selected = false;
}

pair<int, float> TileMapNode::intersect(const Segment &segment) const {
	float dist = intersection(segment, (Box<float3>)m_bounding_box);
	pair<int, float> out(-1, constant::inf);

	if(dist == constant::inf)
		return out;

	for(int i = 0; i < (int)m_instances.size(); i++) {
		const TileInstance &inst = m_instances[i];
		FBox box = (FBox)inst.boundingBox();
		float dist = intersection(segment, box);
		if(dist < out.second) {
			out.first = i;
			out.second = dist;
		}
	}

	return out;
}

int TileMapNode::pixelIntersect(const int2 &screen_pos) const {
	if(!m_screen_rect.isInside(screen_pos))
		return -1;
	
	int best = -1;
	FBox best_box;

	for(int i = 0; i < (int)m_instances.size(); i++) {
		const TileInstance &inst = m_instances[i];
		if(!inst.screenRect().isInside(screen_pos))
			continue;
		if(!inst.m_tile->testPixel(screen_pos - worldToScreen(inst.pos())))
			continue;

		FBox box = (FBox)inst.boundingBox();

		if(best == -1 || drawingOrder(box, best_box) == 1) {
			best = i;
			best_box = box;
		}
	}

	return best;
}
void TileMap::resize(int2 newSize) {
	clear();

	//TODO: properly covert m_nodes to new coordinates
	m_size = int2(newSize.x / Node::size_x, newSize.y / Node::size_z);
	m_nodes.resize(m_size.x * m_size.y);
}

void TileMap::clear() {
	m_size = int2(0, 0);
	m_nodes.clear();
}

void TileMap::loadFromXML(const XMLDocument &doc) {
	XMLNode mnode = doc.child("map");
	ASSERT(mnode);

	int2 size(mnode.intAttrib("size_x"), mnode.intAttrib("size_y"));
	ASSERT(size.x > 0 && size.y > 0 && size.x <= 16 * 1024 && size.y <= 16 * 1024);
	resize(size);

	std::map<int, const gfx::Tile*> tile_indices; //TODO: convert to vector

	XMLNode tnode = doc.child("tile");
	while(tnode) {
		tile_indices[tnode.intAttrib("id")] = &*gfx::Tile::mgr[tnode.value()];
		tnode = tnode.sibling("tile"); 
	}

	XMLNode inode = doc.child("instance");
	while(inode) {
		int id = inode.intAttrib("id");
		int3 pos(inode.intAttrib("pos_x"), inode.intAttrib("pos_y"), inode.intAttrib("pos_z"));
		auto it = tile_indices.find(id);
		ASSERT(it != tile_indices.end());

		addTile(*it->second, pos, false);

		inode = inode.sibling("instance");
	}
}

void TileMap::saveToXML(XMLDocument &doc) const {
	std::map<const gfx::Tile*, int> tile_indices;

	//TODO: dodac jakies sortowanie, tak zeby minimalne zmiany w TileMapie
	// nie powodowały gigantycznych zmian w generowanym XMLu
	XMLNode mnode = doc.addChild("map");
	mnode.addAttrib("size_x", m_size.x * Node::size_x);
	mnode.addAttrib("size_y", m_size.y * Node::size_z);

	for(int n = 0; n < (int)m_nodes.size(); n++) {
		const TileMapNode &node = m_nodes[n];
		for(int i = 0; i < (int)node.m_instances.size(); i++) {
			const gfx::Tile *tile = node.m_instances[i].m_tile;
			auto it = tile_indices.find(tile);
			if(it == tile_indices.end())
				tile_indices[tile] = (int)tile_indices.size();
		}
	}

	for(auto it = tile_indices.begin(); it != tile_indices.end(); ++it) {
		XMLNode node = doc.addChild("tile", doc.own(it->first->name.c_str()));
		node.addAttrib("id", it->second);
	}

	for(int n = 0; n < (int)m_nodes.size(); n++) {
		const TileMapNode &node = m_nodes[n];
		int3 node_pos = nodePos(n);

		for(int i = 0; i < (int)node.m_instances.size(); i++) {
			const TileInstance &inst = node.m_instances[i];
			int id = tile_indices[inst.m_tile];
			int3 pos = node_pos + inst.pos();
			XMLNode node = doc.addChild("instance");
			node.addAttrib("id", id);
			node.addAttrib("pos_x", pos.x);
			node.addAttrib("pos_y", pos.y);
			node.addAttrib("pos_z", pos.z);
		}
	}
}

IBox TileMap::boundingBox() const {
	return IBox(0, 0, 0, m_size.x * Node::size_x, 64, m_size.y * Node::size_z);
}

void TileMap::addToRender(gfx::SceneRenderer &out) const {
	int vNodes = 0, vTiles = 0;

	IRect view = out.targetRect();

	for(uint n = 0; n < m_nodes.size(); n++) {
		const Node &node = m_nodes[n];
		if(!node.instanceCount())
			continue;

		int3 node_pos = nodePos(n);
		IRect screen_rect = node.screenRect() + worldToScreen(node_pos);

		if(!areOverlapping(screen_rect, view))
			continue;
		vNodes++;

		for(uint i = 0; i < node.m_instances.size(); i++) {
			const TileInstance &instance = node.m_instances[i];
			const gfx::Tile *tile = instance.m_tile;
			int3 pos = instance.pos() + node_pos;
			
			if(!tile->deviceTexture())
				const_cast<gfx::Tile*>(tile)->loadDeviceTexture();

			tile->addToRender(out, pos);
			if(instance.isSelected())
				out.addBox(IBox(pos, pos + tile->bboxSize()));
			vTiles++;
		}
	}
	
//	Profiler::updateCounter("rendered nodes", vNodes);
//	Profiler::updateCounter("rendered tiles", vTiles);
}

void TileMap::addTile(const gfx::Tile &tile, int3 pos, bool test_for_collision) {
	if(test_for_collision && isOverlapping(IBox(pos, pos + tile.bboxSize())))
		return;

	int2 nodeCoord(pos.x / Node::size_x, pos.z / Node::size_z);
	int3 node_pos = pos - int3(nodeCoord.x * Node::size_x, 0, nodeCoord.y * Node::size_z);

	(*this)(nodeCoord).addTile(tile, node_pos, test_for_collision);
}

bool TileMap::isOverlapping(const IBox &box) const {
	if(!areOverlapping(box, boundingBox()))
			return false;

//	IRect nodeRect(pos.x / Node::size_x, pos.z / Node::size_z,
//					(pos.x + box.x) / Node::size_x, (pos.z + box.x) / Node::size_z);

	//TODO: speed up
	for(uint n = 0; n < m_nodes.size(); n++)
		if(m_nodes[n].isColliding(box - nodePos(n)))
			return true;

	return false;
}

void TileMap::fill(const gfx::Tile &tile, const IBox &box) {
	int3 bbox = tile.bboxSize();

	for(int x = box.min.x; x < box.max.x; x += bbox.x)
		for(int y = box.min.y; y < box.max.y; y += bbox.y)
			for(int z = box.min.z; z < box.max.z; z += bbox.z) {
				try { addTile(tile, int3(x, y, z)); }
				catch(...) { }
			}
}

void PrintBox(IBox box) {
	printf("%d %d %d %d %d %d",
			box.min.x, box.min.y, box.min.z,
			box.max.x, box.max.y, box.max.z);
}

void TileMap::select(const IBox &box, SelectionMode::Type mode) {
	for(uint n = 0, count = m_nodes.size(); n < count; n++)
		m_nodes[n].select(box - nodePos(n), mode);
	//TODO: faster selection for other modes
}

const TileInstance *TileMap::at(int3 pos) const {
	int id = pos.x / Node::size_x + (pos.z / Node::size_z) * m_size.x;
	DASSERT(id >= 0 && id < (int)m_nodes.size());
	//TODO: wywala sie tutaj, jak sie wypelnia na lewym dolnym krancu mapy

	return m_nodes[id].at(pos - nodePos(id));
}

void TileMap::deleteSelected() {
	for(uint n = 0, count = m_nodes.size(); n < count; n++)
		m_nodes[n].deleteSelected();
}

void TileMap::moveSelected(int3 offset) {
	//TODO: write me
}
	
TileMap::Intersection TileMap::intersect(const Segment &segment) const {
	Intersection out;

	for(int n = 0; n < (int)m_nodes.size(); n++) {
		Segment local_segment(Ray(segment.origin() - nodePos(n), segment.dir()), segment.min, segment.max);
		const pair<int, float> &isect = m_nodes[n].intersect(local_segment);

		if(isect.first != -1 && isect.second < out.distance) {
			out.node_id = n;
			out.instance_id = isect.first;
			out.distance = isect.second;
		}
	}

	return out;
}

pair<int, int> TileMap::pixelIntersect(const int2 &screen_pos) const {
	pair<int, int> out(-1, -1);
	FBox best_box;

	for(int n = 0; n < (int)m_nodes.size(); n++) {
		const TileMapNode &node = m_nodes[n];
		int inst_id = node.pixelIntersect(screen_pos - worldToScreen(nodePos(n)));

		if(inst_id != -1) {
			FBox box(node(inst_id).boundingBox() + nodePos(n));

		   	if(out.first == -1 || drawingOrder(box, best_box) == 1) {
				out.first = n;
				out.second = inst_id;
				best_box = box;
			}
		}
	}

	return out;
}
