#include "tile_map.h"
#include "gfx/tile.h"
#include "gfx/device.h"
#include <algorithm>
#include "sys/profiler.h"

TileInstance::TileInstance(const gfx::Tile *tile, const int3 &pos)
	:flags(0), tile(tile) {
	DAssert(tile);
	setPos(pos);
}


int3 TileInstance::pos() const {
	return int3(int(xz) & 15, int(y), int(xz) >> 4);
}

IBox TileInstance::boundingBox() const {
	int3 pos = this->pos();
	return IBox(pos, pos + tile->bbox);
}

IRect TileInstance::screenRect() const {
	return tile->GetBounds() + int2(WorldToScreen(pos()));
}

void TileInstance::setPos(int3 pos) {
	DAssert(pos.x < TileMapNode::sizeX && pos.y < TileMapNode::sizeY && pos.z < TileMapNode::sizeZ);
	DAssert(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
	xz = u8(pos.x | (pos.z << 4));
	y  = u8(pos.y);
}

void TileMap::resize(int2 newSize) {
	clear();

	//TODO: properly covert nodes to new coordinates
	size = int2(newSize.x / Node::sizeX, newSize.y / Node::sizeZ);
	nodes.resize(size.x * size.y);
}

void TileMap::clear() {
	size = int2(0, 0);
	nodes.clear();
}

bool TileMapNode::isColliding(const IBox &box) const {
	if(!m_instances.size() || !Overlaps(box, m_bounding_box))
		return false;

	for(uint i = 0; i < m_instances.size(); i++) {
		const TileInstance &instance = m_instances[i];
		DAssert(instance.tile);

		int3 tilePos = instance.pos();
		IBox tileBox(tilePos, tilePos + instance.tile->bbox);

		if(Overlaps(tileBox, box))
			return true;
	}

	return false;
}

bool TileMapNode::isInside(const int3 &point) const {
	return IBox(0, 0, 0, sizeX, sizeY, sizeZ).IsInside(point);
}

void TileMapNode::addTile(const gfx::Tile &tile, int3 pos) {
	if(isColliding(IBox(pos, pos + tile.bbox)))
		return;
	
	DAssert(isInside(pos));

	TileInstance inst(&tile, pos);
	m_instances.push_back(inst);

	std::sort(m_instances.begin(), m_instances.end());

	m_screen_rect += inst.screenRect();
	m_bounding_box += inst.boundingBox();
}

void TileMapNode::select(const IBox &box, SelectionMode::Type mode) {
	if(!m_instances.size())
		return;
		
	if(mode == SelectionMode::normal) {
		for(uint i = 0; i < m_instances.size(); i++) {
			TileInstance &instance = m_instances[i];
			instance.select(Overlaps(box, instance.boundingBox()));
		}
	}
	else if(mode == SelectionMode::add) {
		for(uint i = 0; i < m_instances.size(); i++) {
			TileInstance &instance = m_instances[i];
			bool overlaps = Overlaps(box, instance.boundingBox());
			instance.select(overlaps || instance.isSelected());
		}
	}
	else if(mode == SelectionMode::subtract && m_any_selected) {
		for(uint i = 0; i < m_instances.size(); i++) {
			TileInstance &instance = m_instances[i];
			bool overlaps = Overlaps(box, instance.boundingBox());
			instance.select(!overlaps && instance.isSelected());
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
			m_screen_rect += m_instances[n].screenRect();
			m_bounding_box += m_instances[n].boundingBox();
		}

		sort(m_instances.begin(), m_instances.end());
	}
	else {
		m_screen_rect = IRect(0, 0, 0, 0);
		m_bounding_box = IBox(0, 0, 0, 0, 0, 0);
	}
	
	m_any_selected = false;
}

void TileMap::loadFromXML(const XMLDocument &doc) {
	clear();
}

void TileMap::saveToXML(XMLDocument &doc) const {
/*
	sr & size;
	sr & nodes;
	u32 nTiles = tiles.size();
	sr & nTiles;

	if(sr.IsLoading()) {
		tiles.resize(nTiles);
		for(uint n = 0; n < tiles.size(); n++) {
			string name; sr & name;
			auto it = tileDict->find(name);
			if(it == tileDict->end())
				ThrowException("Tile not found: ", name);

			tiles[n] = it->second;
		}
	
		for(uint n = 0; n < tiles.size(); n++)
			tile2Id.insert(std::make_pair(tiles[n], TileId(n)));
	}
	else {
		for(uint n = 0; n < tiles.size(); n++) {
			string tmp = tiles[n]->name;
			sr & tmp;
		}
	}*/
}

IBox TileMap::boundingBox() const {
	return IBox(0, 0, 0, size.x * Node::sizeX, 64, size.y * Node::sizeZ);
}

void TileMap::render(const IRect &view) const {
	PROFILE(tRendering)

	gfx::DTexture::Bind0();
	gfx::DrawBBox(boundingBox());

	int vNodes = 0, vTiles = 0;
	{
		PROFILE(tRenderingPreparation)
	}
	//TODO: selecting visible nodes
	//TODO: sorting tiles
	
	for(uint n = 0; n < nodes.size(); n++) {
		const Node &node = nodes[n];
		if(!node.instanceCount())
			continue;

		int3 node_pos = nodePos(n);
		IRect screenRect = node.screenRect() + WorldToScreen(node_pos);
		// possible error from rounding node & tile positions
		screenRect.min -= int2(2, 2);
		screenRect.max += int2(2, 2);
//		if(!Overlaps(screenRect, view))
//			continue;
		vNodes++;

		for(uint i = 0; i < node.m_instances.size(); i++) {
			const TileInstance &instance = node.m_instances[i];
			const gfx::Tile *tile = instance.tile;
			int3 pos = instance.pos() + node_pos;
			int2 screenPos = WorldToScreen(pos);

			vTiles++;
			tile->Draw(screenPos);
			if(instance.isSelected()) {
				gfx::DTexture::Bind0();
				gfx::DrawBBox(IBox(pos, pos + tile->bbox));
			}
		}
	}

	Profiler::UpdateCounter(Profiler::cRenderedNodes, vNodes);
	Profiler::UpdateCounter(Profiler::cRenderedTiles, vTiles);
}

void TileMap::addTile(const gfx::Tile &tile, int3 pos) {
	if(!testPosition(pos, tile.bbox))
		return;

	int2 nodeCoord(pos.x / Node::sizeX, pos.z / Node::sizeZ);
	(*this)(nodeCoord).addTile(tile, pos - int3(nodeCoord.x * Node::sizeX, 0, nodeCoord.y * Node::sizeZ));
}

bool TileMap::testPosition(int3 pos, int3 box) const {
	IRect nodeRect(pos.x / Node::sizeX, pos.z / Node::sizeZ,
					(pos.x + box.x - 1) / Node::sizeX, (pos.z + box.x - 1) / Node::sizeZ);
	IBox worldBox(pos, pos + box);

	if(pos.x < 0 || pos.y < 0 || pos.z < 0)
		return false;

	if(worldBox.max.x > size.x * Node::sizeX ||
		worldBox.max.y > Node::sizeY || worldBox.max.z > size.y * Node::sizeZ)
		return false;

	for(uint n = 0; n < nodes.size(); n++) {
		IBox bbox(pos, pos + box);
		bbox -= nodePos(n);
		if(nodes[n].isColliding(bbox))
			return false;
	}

	return true;
}

void TileMap::drawPlacingHelpers(const gfx::Tile &tile, int3 pos) const {
	bool collides = !testPosition(pos, tile.bbox);

	Color color = collides? Color(255, 0, 0) : Color(255, 255, 255);

	tile.Draw(int2(WorldToScreen(pos)), color);
	gfx::DTexture::Bind0();
	gfx::DrawBBox(IBox(pos, pos + tile.bbox));
}

void TileMap::drawBoxHelpers(const IBox &box) const {
	gfx::DTexture::Bind0();

	int3 pos = box.min, bbox = box.max - box.min;
	int3 tsize(size.x * Node::sizeX, Node::sizeY, size.y * Node::sizeZ);

	gfx::DrawLine(int3(0, pos.y, pos.z), int3(tsize.x, pos.y, pos.z), Color(0, 255, 0, 127));
	gfx::DrawLine(int3(0, pos.y, pos.z + bbox.z), int3(tsize.x, pos.y, pos.z + bbox.z), Color(0, 255, 0, 127));
	
	gfx::DrawLine(int3(pos.x, pos.y, 0), int3(pos.x, pos.y, tsize.z), Color(0, 255, 0, 127));
	gfx::DrawLine(int3(pos.x + bbox.x, pos.y, 0), int3(pos.x + bbox.x, pos.y, tsize.z), Color(0, 255, 0, 127));

	int3 tpos(pos.x, 0, pos.z);
	gfx::DrawBBox(IBox(tpos, tpos + int3(bbox.x, pos.y, bbox.z)), Color(0, 0, 255, 127));
	
	gfx::DrawLine(int3(0, 0, pos.z), int3(tsize.x, 0, pos.z), Color(0, 0, 255, 127));
	gfx::DrawLine(int3(0, 0, pos.z + bbox.z), int3(tsize.x, 0, pos.z + bbox.z), Color(0, 0, 255, 127));
	
	gfx::DrawLine(int3(pos.x, 0, 0), int3(pos.x, 0, tsize.z), Color(0, 0, 255, 127));
	gfx::DrawLine(int3(pos.x + bbox.x, 0, 0), int3(pos.x + bbox.x, 0, tsize.z), Color(0, 0, 255, 127));
}

void TileMap::fill(const gfx::Tile &tile, const IBox &box) {
	int3 bbox = tile.bbox;

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
	for(uint n = 0, count = nodes.size(); n < count; n++)
		nodes[n].select(box - nodePos(n), mode);
	//TODO: faster selection for other modes
}

void TileMap::deleteSelected() {
	for(uint n = 0, count = nodes.size(); n < count; n++)
		nodes[n].deleteSelected();
}

void TileMap::moveSelected(int3 offset) {
	//TODO: write me
}
