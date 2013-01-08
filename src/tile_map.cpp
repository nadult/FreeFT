#include "tile_map.h"
#include "gfx/tile.h"
#include "gfx/scene_renderer.h"
#include "sys/profiler.h"
#include "sys/xml.h"
#include <algorithm>


void TileMap::resize(const int2 &new_size) {
	TileMap new_map(new_size);
	for(int n = 0; n < size(); n++) {
		const ObjectDef &obj = Grid::operator[](n);
		if(obj.ptr && new_map.isInside(obj.bbox))
			new_map.Grid::add(obj);
	}
	Grid::swap(new_map);
}

int TileMap::add(const gfx::Tile *tile, const int3 &pos) {
	DASSERT(tile);

	FBox bbox(pos, pos + tile->bboxSize());
	IRect rect = tile->rect() + worldToScreen(pos);
//	ASSERT(findAny(bbox) == -1);
	return Grid::add(ObjectDef(tile, bbox, rect, -1));
}

void TileMap::loadFromXML(const XMLDocument &doc) {
	XMLNode main_node = doc.child("tile_map");
	ASSERT(main_node);

	clear();

	int2 size = main_node.int2Attrib("size");
	ASSERT(size.x > 0 && size.y > 0 && size.x <= 16 * 1024 && size.y <= 16 * 1024);
	resize(size);

	XMLNode tnode = main_node.child("tile");
	while(tnode) {
		const gfx::Tile *tile = &*gfx::Tile::mgr[tnode.attrib("name")];
		XMLNode inode = tnode.child("i");
		while(inode) {
			int3 pos = inode.int3Attrib("pos");
			add(tile, pos);
			inode = inode.sibling("i");
		}
		tnode = tnode.sibling("tile");
	}
}

void TileMap::saveToXML(XMLDocument &doc) const {
	XMLNode main_node = doc.addChild("tile_map");
	main_node.addAttrib("size", dimensions());

	std::vector<int> indices;
	indices.reserve(size());
	for(int n = 0; n < size(); n++)
		if((*this)[n].ptr)
			indices.push_back(n);

	std::sort(indices.begin(), indices.end(), [this](int a, int b)
			{ return (*this)[a].ptr->name < (*this)[b].ptr->name; } );
	
	const gfx::Tile *prev = nullptr;
	XMLNode tile_node;

	for(int n = 0; n < (int)indices.size(); n++) {
		const TileDef &object = (*this)[indices[n]];
		if(object.ptr != prev) {
			tile_node = main_node.addChild("tile");
			ASSERT(!object.ptr->name.empty());
		   	tile_node.addAttrib("name", doc.own(object.ptr->name.c_str()));
			prev = object.ptr;
		}

		XMLNode instance = tile_node.addChild("i");
		instance.addAttrib("pos", int3(object.bbox.min));
	}
}
