#ifndef TILE_MAP_H
#define TILE_MAP_H

#include "base.h"
#include <map>

namespace gfx { class Tile; class SceneRenderer; };

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
	const gfx::Tile *m_tile;
	u8 m_xz;
	u8 m_y;
	u16 m_flags;

	bool isSelected() const { return m_flags & InstanceFlags::isSelected; }
	void select(bool select)
		{ m_flags = select? m_flags | InstanceFlags::isSelected : m_flags & ~InstanceFlags::isSelected; }

	TileInstance() :m_tile(0), m_flags(0) { }
	TileInstance(const gfx::Tile *tile, const int3 &pos);

	int3 pos() const;
	void setPos(int3 pos);

	// in node coordinates
	IBox boundingBox() const;
	// in node coordinates
	IRect screenRect() const;

	bool operator<(const TileInstance &rhs) const {
		return m_y == rhs.m_y? m_xz < rhs.m_xz : m_y < rhs.m_y;
	}
};

class TileMap;

struct TileMapNode {
	TileMapNode()
		:m_bounding_box(0, 0, 0, 0, 0, 0), m_screen_rect(0, 0, 0, 0), m_any_selected(false) { }

	enum {
		size_x = 16,
		size_y = 256,
		size_z = 16,
	};

	void addTile(const gfx::Tile&, int3 pos, bool test_for_collision);

	bool isColliding(const IBox &box) const;
	bool isInside(const int3 &point) const;

	const TileInstance *at(int3 pos) const;
	void select(const IBox &box, SelectionMode::Type);
	void deleteSelected();

	vector<TileInstance> m_instances;

	IRect screenRect() const { return m_screen_rect; }
	IBox  boundingBox() const { return m_bounding_box; }

	const TileInstance& operator()(int idx) const { return m_instances[idx]; }
	int   instanceCount() const { return m_instances.size(); }

protected:
	IBox m_bounding_box; // in local coordinates
	IRect m_screen_rect; // in local coords
	bool m_any_selected;
};

// Acceleration structure that optimizes rendering
// use different structure for collisions
class TileMap {
public:
	typedef TileMapNode Node;

	int2 size() const { return int2(m_size.x * Node::size_x, m_size.y * Node::size_z); }

	void resize(int2 size);
	void clear();
	void addToRender(gfx::SceneRenderer&) const;

	// in TileMap coordinates, that is each unit is single TileMapNode
	Node& operator()(int2 pos) { return m_nodes[pos.x + pos.y * m_size.x]; }
	const Node& operator()(int2 pos) const { return m_nodes[pos.x + pos.y * m_size.x]; }

	const Node& operator()(int idx) const { return m_nodes[idx]; }
	int nodeCount() const { return (int)m_nodes.size(); }
	int nodeCountX() const { return m_size.x; }
	int nodeCountZ() const { return m_size.y; }

	IBox boundingBox() const;
	int3 nodePos(int id) const { return int3((id % m_size.x) * Node::size_x, 0, (id / m_size.x) * Node::size_z); }

	// Tile has to exist as long as TileMap does
	void addTile(const gfx::Tile &tile, int3 pos, bool test_for_collision = true);
	void fill(const gfx::Tile &tile, const IBox &box);
	void select(const IBox &box, SelectionMode::Type);
	
	const TileInstance *at(int3 pos) const;

	void deleteSelected();
	void moveSelected(int3 offset);

	// returns true if box collides with any of the tiles
	bool isOverlapping(const IBox &box) const;

	void loadFromXML(const XMLDocument&);
	void saveToXML(XMLDocument&) const;

protected:
	vector<Node> m_nodes;
	int2 m_size;
};

SERIALIZE_AS_POD(TileInstance);

#endif
