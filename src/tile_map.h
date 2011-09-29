#include "base.h"
#include <map>

class TileMapEditor;

namespace gfx { class Tile; };

// Acceleration structure that optimizes rendering
// user different structure for collisions
class TileMap {
public:
	typedef u16 TileId;
	enum { maxTiles = 65536 };

	struct Node {
		enum {
			sizeX = 16,
			sizeY = 256,
			sizeZ = 16,
		};

		struct Instance {
			u8 xz;
			u8 y;
			TileId id;

			int3 GetPos() const { return int3(int(xz) & 15, int(y), int(xz) >> 4); }
			void SetPos(int3 pos) {
				Assert(pos.x < sizeX && pos.y < sizeY && pos.z < sizeZ);
				Assert(pos.x >= 0 && pos.y >= 0 && pos.z >= 0);
				xz = u8(pos.x | (pos.z << 4));
				y  = u8(pos.y);
			}

			bool operator<(const Instance &rhs) const {
				return y == rhs.y? xz < rhs.xz : y < rhs.y;
			}
		};

		vector<Instance> instances;
	};

	void Resize(int2 size);
	void Clear();
	void Render(int2 viewPos, bool showBBoxes) const;

	Node& operator()(int2 pos) { return nodes[pos.x + pos.y * size.x]; }
	const Node& operator()(int2 pos) const { return nodes[pos.x + pos.y * size.x]; }

	friend class TileMapEditor;

protected:
	vector<Node> nodes;
	vector<gfx::Tile*> tiles;
	int2 size;
};

class TileMapEditor {
public:
	typedef TileMap::TileId TileId;
	typedef TileMap::Node Node;

	TileMapEditor(TileMap &map);

	void AddTile(gfx::Tile *tile, int3 pos);

protected:
	TileMap &tileMap;
	std::map<gfx::Tile*, TileId> tile2Id;
};
