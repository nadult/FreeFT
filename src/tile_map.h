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

	bool IsSelected() const { return flags & InstanceFlags::isSelected; }
	void Select(bool select) { flags = select? flags | InstanceFlags::isSelected : flags & ~InstanceFlags::isSelected; }

	TileInstance() :tile(0), flags(0) { }
	TileInstance(const gfx::Tile *tile, const int3 &pos);

	int3 GetPos() const;
	void SetPos(int3 pos);

	// in node coordinates
	IBox GetBoundingBox() const;
	// in node coordinates
	IRect GetScreenRect() const;

	bool operator<(const TileInstance &rhs) const {
		return y == rhs.y? xz < rhs.xz : y < rhs.y;
	}
};

class TileMap;

struct TileMapNode {
	TileMapNode()
		:boundingBox(0, 0, 0, 0, 0, 0), screenRect(0, 0, 0, 0), anySelected(false) { }

	enum {
		sizeX = 16,
		sizeY = 256,
		sizeZ = 16,
	};

	void Serialize(Serializer&);
	void AddTile(const gfx::Tile&, int3 pos);

	bool IsColliding(const IBox &box) const;
	bool IsInside(const int3 &point) const;

	void Select(const IBox &box, SelectionMode::Type);
	void DeleteSelected();

	vector<TileInstance> instances;

	IRect GetScreenRect() const { return screenRect; }
	IBox  GetBoudingBox() const { return boundingBox; }
	int   GetInstancesCount() const { return instances.size(); }

protected:
//	friend class TileMap; //TODO: remove

	IBox boundingBox; // in local coordinates
	IRect screenRect; // in local coords
	bool anySelected;
};

// Acceleration structure that optimizes rendering
// user different structure for collisions
class TileMap {
public:
	typedef TileMapNode Node;

	void Resize(int2 size);
	void Clear();
	void Render(const IRect &view) const;
//	void Serialize(Serializer&, std::map<string, const gfx::Tile*> *tileDict = NULL);

	Node& operator()(int2 pos) { return nodes[pos.x + pos.y * size.x]; }
	const Node& operator()(int2 pos) const { return nodes[pos.x + pos.y * size.x]; }

	int3 GetNodePos(int id) const { return int3((id % size.x) * Node::sizeX, 0, (id / size.x) * Node::sizeZ); }

	// Tile has to exist as long as TileMap does
	void AddTile(const gfx::Tile &tile, int3 pos);
	void Fill(const gfx::Tile &tile, const IBox &box);
	void Select(const IBox &box, SelectionMode::Type);

	void DrawPlacingHelpers(const gfx::Tile &tile, int3 pos) const;
	void DrawBoxHelpers(const IBox &box) const;
	void DeleteSelected();
	void MoveSelected(int3 offset);

	// returns true if box doesn't collide with any of the tiles
	bool TestPosition(int3 pos, int3 box) const;

protected:
	vector<Node> nodes;
	int2 size;
};

SERIALIZE_AS_POD(TileInstance);
