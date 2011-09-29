#include "tile_map.h"
#include "gfx/tile.h"
#include "gfx/device.h"

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

void TileMap::Render(int2 viewPos, bool showBBoxes) const {
	if(showBBoxes) {
		gfx::DTexture::Bind0();
		gfx::DrawBBox(-viewPos, int3(-size.x * Node::sizeX, 64, -size.y * Node::sizeZ));
	}

	for(uint n = 0; n < nodes.size(); n++) {
		const Node &node = nodes[n];
		int3 nodePos((n % size.x) * Node::sizeX, 0, (n / size.x) * Node::sizeZ);

		for(uint i = 0; i < node.instances.size(); i++) {
			const Node::Instance &instance = node.instances[i];
			gfx::Tile *tile = tiles[instance.id];
			int3 pos = instance.GetPos() + nodePos;
			int2 screenPos = int2(WorldToScreen(pos));

			tile->Draw(screenPos - viewPos);
			if(showBBoxes) {
				gfx::DTexture::Bind0();
				gfx::DrawBBox(screenPos - viewPos, tile->bbox);
			}
		}
	}
}

TileMapEditor::TileMapEditor(TileMap &tileMap) :tileMap(tileMap) {
	for(uint n = 0; n < tileMap.tiles.size(); n++)
		tile2Id.insert(std::make_pair(tileMap.tiles[n], TileId(n)));
}

void TileMapEditor::AddTile(gfx::Tile *tile, int3 pos) {
	auto it = tile2Id.find(tile);
	if(it == tile2Id.end()) {
		if(tileMap.tiles.size() == TileMap::maxTiles)
			ThrowException("Maximum tile limit reached (", TileMap::maxTiles, ")!");

		tileMap.tiles.push_back(tile);
		it = tile2Id.insert(std::make_pair(tile, TileId(tileMap.tiles.size() - 1))).first;
	}

	int2 nodePos(pos.x / Node::sizeX, pos.z / Node::sizeZ);
	int2 nodeOffset = int2(pos.x, pos.z) - int2(nodePos.x * Node::sizeX, nodePos.y * Node::sizeZ);
	int2 size = tileMap.size;

	Assert(nodePos.x >= 0 && nodePos.y >= 0);
	Assert(nodePos.x < size.x && nodePos.y < size.y);
	Assert(nodeOffset.x < Node::sizeX && nodeOffset.y < Node::sizeY);
	Assert(pos.y >= 0 && pos.y < Node::sizeY);

	Node::Instance newInst;
	newInst.id = TileId(it->second);
	newInst.SetPos(int3(nodeOffset.x, pos.y, nodeOffset.y));
	tileMap(nodePos).instances.push_back(newInst);
	
	printf("Pos: %d %d %d -> %d %d %d\n", pos.x, pos.y, pos.z,
			nodePos.x * Node::sizeX + newInst.GetPos().x, newInst.GetPos().y, nodePos.y * Node::sizeZ + newInst.GetPos().z);
}
