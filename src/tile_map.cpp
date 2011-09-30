#include "tile_map.h"
#include "gfx/tile.h"
#include "gfx/device.h"
#include <algorithm>
#include "sys/profiler.h"

void TileMap::Resize(int2 newSize) {
	Clear();

	//TODO: properly covert nodes to new coordinates
	size = int2(newSize.x / Node::sizeX, newSize.y / Node::sizeZ);
	nodes.resize(size.x * size.y);
}

void TileMap::Clear() {
	size = int2(0, 0);
	nodes.clear();
	tiles.clear();
}

void TileMap::Node::Serialize(Serializer &sr) {
	sr & screenBounds & instances;
}

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
	}
	else {
		for(uint n = 0; n < tiles.size(); n++) {
			string tmp = tiles[n]->name;
			sr & tmp;
		}
	}
}

void TileMap::Render(const IRect &view, bool showBBoxes) const {
	PROFILE(tRendering)

	if(showBBoxes) {
		gfx::DTexture::Bind0();
		gfx::DrawBBox(-view.min, int3(-size.x * Node::sizeX, 64, -size.y * Node::sizeZ));
	}

	int vNodes = 0, vTiles = 0;
	{
		PROFILE(tRenderingPreparation)
	}
	//TODO: selecting visible nodes
	//TODO: sorting tiles
	
	for(uint n = 0; n < nodes.size(); n++) {
		const Node &node = nodes[n];
		int3 nodePos((n % size.x) * Node::sizeX, 0, (n / size.x) * Node::sizeZ);
		IRect screenBounds = node.screenBounds + int2(WorldToScreen(nodePos));
		// possible error from rounding node & tile positions
		screenBounds.min -= int2(2, 2);
		screenBounds.max += int2(2, 2);
		if(!Overlaps(screenBounds, view))
			continue;
		vNodes++;

		for(uint i = 0; i < node.instances.size(); i++) {
			const Node::Instance &instance = node.instances[i];
			const gfx::Tile *tile = tiles[instance.id];
			int3 pos = instance.GetPos() + nodePos;
			int2 screenPos = int2(WorldToScreen(pos));

			vTiles++;
			tile->Draw(screenPos - view.min);
			if(showBBoxes) {
				gfx::DTexture::Bind0();
				gfx::DrawBBox(screenPos - view.min, tile->bbox);
			}
		}
	}

	Profiler::UpdateCounter(Profiler::cRenderedNodes, vNodes);
	Profiler::UpdateCounter(Profiler::cRenderedTiles, vTiles);
}

TileMapEditor::TileMapEditor(TileMap &tileMap) :tileMap(&tileMap) {
	for(uint n = 0; n < tileMap.tiles.size(); n++)
		tile2Id.insert(std::make_pair(tileMap.tiles[n], TileId(n)));
}

void TileMapEditor::AddTile(const gfx::Tile &rTile, int3 pos) {
	const gfx::Tile *tile = &rTile;

	auto it = tile2Id.find(tile);
	if(it == tile2Id.end()) {
		if(tileMap->tiles.size() == TileMap::maxTiles)
			ThrowException("Maximum tile limit reached (", TileMap::maxTiles, ")!");

		tileMap->tiles.push_back(tile);
		it = tile2Id.insert(std::make_pair(tile, TileId(tileMap->tiles.size() - 1))).first;
	}

	int2 nodePos(pos.x / Node::sizeX, pos.z / Node::sizeZ);
	int2 nodeOffset = int2(pos.x, pos.z) - int2(nodePos.x * Node::sizeX, nodePos.y * Node::sizeZ);
	int2 size = tileMap->size;

	Assert(nodePos.x >= 0 && nodePos.y >= 0);
	Assert(nodePos.x < size.x && nodePos.y < size.y);
	Assert(nodeOffset.x < Node::sizeX && nodeOffset.y < Node::sizeY);
	Assert(pos.y >= 0 && pos.y < Node::sizeY);

	Node::Instance newInst;
	newInst.id = TileId(it->second);
	newInst.SetPos(int3(nodeOffset.x, pos.y, nodeOffset.y));
	
	Node &node = (*tileMap)(nodePos);
	node.instances.push_back(newInst);
	std::sort(node.instances.begin(), node.instances.end());
	node.screenBounds += tile->GetBounds() + int2(WorldToScreen(newInst.GetPos()));
}

void TileMapEditor::Fill(const gfx::Tile &tile, int3 min, int3 max) {
	int3 bbox = tile.bbox;

	for(int x = min.x; x < max.x; x += bbox.x)
		for(int y = min.y; y < max.y; y += bbox.y)
			for(int z = min.z; z < max.z; z += bbox.z) {
				try { AddTile(tile, int3(x, y, z)); }
				catch(...) { }
			}
}
