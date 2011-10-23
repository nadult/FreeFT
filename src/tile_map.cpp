#include "tile_map.h"
#include "gfx/tile.h"
#include "gfx/device.h"
#include <algorithm>
#include "sys/profiler.h"

TileInstance::TileInstance(const gfx::Tile *tile, const int3 &pos)
	:flags(0), tile(tile) {
	Assert(tile);
	SetPos(pos);
}


int3 TileInstance::GetPos() const {
	return int3(int(xz) & 15, int(y), int(xz) >> 4);
}

IBox TileInstance::GetBoundingBox() const {
	int3 pos = GetPos();
	return IBox(pos, pos + tile->bbox);
}

IRect TileInstance::GetScreenRect() const {
	return tile->GetBounds() + int2(WorldToScreen(GetPos()));
}

void TileInstance::SetPos(int3 pos) {
	Assert(pos.x < TileMapNode::sizeX && pos.y < TileMapNode::sizeY && pos.z < TileMapNode::sizeZ);
	Assert(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
	xz = u8(pos.x | (pos.z << 4));
	y  = u8(pos.y);
}

void TileMap::Resize(int2 newSize) {
	Clear();

	//TODO: properly covert nodes to new coordinates
	size = int2(newSize.x / Node::sizeX, newSize.y / Node::sizeZ);
	nodes.resize(size.x * size.y);
}

void TileMap::Clear() {
	size = int2(0, 0);
	nodes.clear();
}

void TileMap::Node::Serialize(Serializer &sr) {
	sr & screenRect & boundingBox & instances;
}

bool TileMapNode::IsColliding(const IBox &box) const {
	if(!instances.size() || !Overlaps(box, boundingBox))
		return false;

	for(uint i = 0; i < instances.size(); i++) {
		const TileInstance &instance = instances[i];
		Assert(instance.tile);

		int3 tilePos = instance.GetPos();
		IBox tileBox(tilePos, tilePos + instance.tile->bbox);

		if(Overlaps(tileBox, box))
			return true;
	}

	return false;
}

bool TileMapNode::IsInside(const int3 &point) const {
	return IBox(0, 0, 0, sizeX, sizeY, sizeZ).IsInside(point);
}

void TileMapNode::AddTile(const gfx::Tile &tile, int3 pos) {
	if(IsColliding(IBox(pos, pos + tile.bbox)))
		return;
	
	Assert(IsInside(pos));

	TileInstance inst(&tile, pos);
	instances.push_back(inst);

	std::sort(instances.begin(), instances.end());

	screenRect += inst.GetScreenRect();
	boundingBox += inst.GetBoundingBox();
}

void TileMapNode::Select(const IBox &box, SelectionMode::Type mode) {
	if(!instances.size())
		return;
		
	if(mode == SelectionMode::normal) {
		for(uint i = 0; i < instances.size(); i++) {
			TileInstance &instance = instances[i];
			instance.Select(Overlaps(box, instance.GetBoundingBox()));
		}
	}
	else if(mode == SelectionMode::add) {
		for(uint i = 0; i < instances.size(); i++) {
			TileInstance &instance = instances[i];
			bool overlaps = Overlaps(box, instance.GetBoundingBox());
			instance.Select(overlaps || instance.IsSelected());
		}
	}
	else if(mode == SelectionMode::subtract && anySelected) {
		for(uint i = 0; i < instances.size(); i++) {
			TileInstance &instance = instances[i];
			bool overlaps = Overlaps(box, instance.GetBoundingBox());
			instance.Select(!overlaps && instance.IsSelected());
		}
	}
	
	anySelected = false;
	for(uint i = 0; i < instances.size(); i++)
		anySelected |= instances[i].IsSelected();
}

void TileMapNode::DeleteSelected() {
	if(!anySelected)
		return;

	for(uint i = 0; i < instances.size(); i++)
		if(instances[i].IsSelected()) {
			instances[i--] = instances.back();
			instances.pop_back();
		}

	if(instances.size()) {
		screenRect = instances[0].GetScreenRect();
		boundingBox = instances[0].GetBoundingBox();

		for(uint n = 1; n < instances.size(); n++) {
			screenRect += instances[n].GetScreenRect();
			boundingBox += instances[n].GetBoundingBox();
		}

		sort(instances.begin(), instances.end());
	}
	else {
		screenRect = IRect(0, 0, 0, 0);
		boundingBox = IBox(0, 0, 0, 0, 0, 0);
	}
	
	anySelected = false;
}

/*
void TileMap::Serialize(Serializer &sr, std::map<string, const gfx::Tile*> *tileDict) {
	if(sr.IsLoading()) {
		Assert(tileDict);
		Clear();
	}

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
	}
}*/

void TileMap::Render(const IRect &view) const {
	PROFILE(tRendering)

	gfx::DTexture::Bind0();
	gfx::DrawBBox(IBox(0, 0, 0, size.x * Node::sizeX, 64, size.y * Node::sizeZ));

	int vNodes = 0, vTiles = 0;
	{
		PROFILE(tRenderingPreparation)
	}
	//TODO: selecting visible nodes
	//TODO: sorting tiles
	
	for(uint n = 0; n < nodes.size(); n++) {
		const Node &node = nodes[n];
		if(!node.GetInstancesCount())
			continue;

		int3 nodePos = GetNodePos(n);
		IRect screenRect = node.GetScreenRect() + int2(WorldToScreen(nodePos));
		// possible error from rounding node & tile positions
		screenRect.min -= int2(2, 2);
		screenRect.max += int2(2, 2);
		if(!Overlaps(screenRect, view))
			continue;
		vNodes++;

		for(uint i = 0; i < node.instances.size(); i++) {
			const TileInstance &instance = node.instances[i];
			const gfx::Tile *tile = instance.tile;
			int3 pos = instance.GetPos() + nodePos;
			int2 screenPos = int2(WorldToScreen(pos));

			vTiles++;
			tile->Draw(screenPos);
			if(instance.IsSelected()) {
				gfx::DTexture::Bind0();
				gfx::DrawBBox(IBox(pos, pos + tile->bbox));
			}
		}
	}

	Profiler::UpdateCounter(Profiler::cRenderedNodes, vNodes);
	Profiler::UpdateCounter(Profiler::cRenderedTiles, vTiles);
}

void TileMap::AddTile(const gfx::Tile &tile, int3 pos) {
	if(!TestPosition(pos, tile.bbox))
		return;

	int2 nodeCoord(pos.x / Node::sizeX, pos.z / Node::sizeZ);
	(*this)(nodeCoord).AddTile(tile, pos - int3(nodeCoord.x * Node::sizeX, 0, nodeCoord.y * Node::sizeZ));
}

bool TileMap::TestPosition(int3 pos, int3 box) const {
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
		bbox -= GetNodePos(n);
		if(nodes[n].IsColliding(bbox))
			return false;
	}

	return true;
}

void TileMap::DrawPlacingHelpers(const gfx::Tile &tile, int3 pos) const {
	bool collides = !TestPosition(pos, tile.bbox);

	Color color = collides? Color(255, 0, 0) : Color(255, 255, 255);

	tile.Draw(int2(WorldToScreen(pos)), color);
	gfx::DTexture::Bind0();
	gfx::DrawBBox(IBox(pos, pos + tile.bbox));
}

void TileMap::DrawBoxHelpers(const IBox &box) const {
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

void TileMap::Fill(const gfx::Tile &tile, const IBox &box) {
	int3 bbox = tile.bbox;

	for(int x = box.min.x; x < box.max.x; x += bbox.x)
		for(int y = box.min.y; y < box.max.y; y += bbox.y)
			for(int z = box.min.z; z < box.max.z; z += bbox.z) {
				try { AddTile(tile, int3(x, y, z)); }
				catch(...) { }
			}
}

void PrintBox(IBox box) {
	printf("%d %d %d %d %d %d",
			box.min.x, box.min.y, box.min.z,
			box.max.x, box.max.y, box.max.z);
}

void TileMap::Select(const IBox &box, SelectionMode::Type mode) {
	for(uint n = 0, count = nodes.size(); n < count; n++)
		nodes[n].Select(box - GetNodePos(n), mode);
	//TODO: faster selection for other modes
}

void TileMap::DeleteSelected() {
	for(uint n = 0, count = nodes.size(); n < count; n++)
		nodes[n].DeleteSelected();
}

void TileMap::MoveSelected(int3 offset) {
	//TODO: write me
}
