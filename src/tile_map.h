#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "base.h"
#include <map>

namespace gfx { class Tile; };

namespace SelectionMode {
	enum Type {
		normal,
		add,
		subtract,
	};
};

namespace InstanceFlags {
	enum Type {
		isSelected = 1,
		seeThrough = 2,
	};
};

typedef u16 TileId;

struct TileInstance {
	const gfx::Tile *tile;
	u8 xz;
	u8 y;
	u16 flags;

	bool isSelected() const { return flags & InstanceFlags::isSelected; }
	void select(bool select) { flags = select? flags | InstanceFlags::isSelected : flags & ~InstanceFlags::isSelected; }

	TileInstance() :tile(0), flags(0) { }
	TileInstance(const gfx::Tile *tile, const int3 &pos);

	int3 pos() const;
	void setPos(int3 pos);

	// in node coordinates
	IBox boundingBox() const;
	// in node coordinates
	IRect screenRect() const;

	bool operator<(const TileInstance &rhs) const {
		return y == rhs.y? xz < rhs.xz : y < rhs.y;
	}
};

class TileMap;

struct TileMapNode {
	TileMapNode()
		:m_bounding_box(0, 0, 0, 0, 0, 0), m_screen_rect(0, 0, 0, 0), m_any_selected(false) { }

	enum {
		sizeX = 16,
		sizeY = 256,
		sizeZ = 16,
	};

	void addTile(const gfx::Tile&, int3 pos);

	bool isColliding(const IBox &box) const;
	bool isInside(const int3 &point) const;

	void select(const IBox &box, SelectionMode::Type);
	void deleteSelected();

	vector<TileInstance> m_instances;

	IRect screenRect() const { return m_screen_rect; }
	IBox  boudingBox() const { return m_bounding_box; }
	int   instanceCount() const { return m_instances.size(); }

protected:
	IBox m_bounding_box; // in local coordinates
	IRect m_screen_rect; // in local coords
	bool m_any_selected;
};

// Acceleration structure that optimizes rendering
// user different structure for collisions
class TileMap {
public:
	typedef TileMapNode Node;

	void resize(int2 size);
	void clear();
	void render(const IRect &view) const;

	Node& operator()(int2 pos) { return nodes[pos.x + pos.y * size.x]; }
	const Node& operator()(int2 pos) const { return nodes[pos.x + pos.y * size.x]; }

	IBox boundingBox() const;
	int3 nodePos(int id) const { return int3((id % size.x) * Node::sizeX, 0, (id / size.x) * Node::sizeZ); }

	// Tile has to exist as long as TileMap does
	void addTile(const gfx::Tile &tile, int3 pos);
	void fill(const gfx::Tile &tile, const IBox &box);
	void select(const IBox &box, SelectionMode::Type);

	void drawPlacingHelpers(const gfx::Tile &tile, int3 pos) const;
	void drawBoxHelpers(const IBox &box) const;
	void deleteSelected();
	void moveSelected(int3 offset);

	// returns true if box doesn't collide with any of the tiles
	bool testPosition(int3 pos, int3 box) const;

	void loadFromXML(const XMLDocument&);
	void saveToXML(XMLDocument&) const;

protected:
	vector<Node> nodes;
	int2 size;
};

SERIALIZE_AS_POD(TileInstance);

#endif
