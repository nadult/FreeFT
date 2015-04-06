/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef BASE_H
#define BASE_H

#include "fwk.h"

using namespace fwk;

struct MoveVector {
	MoveVector(const int2 &start, const int2 &end);
	MoveVector();

	int2 vec;
	int dx, dy, ddiag;
};

// TODO: support for overlapping boxes
template <class Type3> int drawingOrder(const Box<Type3> &a, const Box<Type3> &b) {
	// DASSERT(!areOverlapping(box, box));

	int y_ret = a.max.y <= b.min.y ? -1 : b.max.y <= a.min.y ? 1 : 0;
	if(y_ret)
		return y_ret;

	int x_ret = a.max.x <= b.min.x ? -1 : b.max.x <= a.min.x ? 1 : 0;
	if(x_ret)
		return x_ret;

	int z_ret = a.max.z <= b.min.z ? -1 : b.max.z <= a.min.z ? 1 : 0;
	return z_ret;
}

const float2 worldToScreen(const float3 &pos);
const int2 worldToScreen(const int3 &pos);

const float2 screenToWorld(const float2 &pos);
const int2 screenToWorld(const int2 &pos);

const Ray screenRay(const int2 &screen_pos);

template <class Type3> const Rect<decltype(Type3().xy())> worldToScreen(const Box<Type3> &bbox) {
	typedef decltype(Type3().xy()) Type2;
	Type2 corners[4] = {worldToScreen(Type3(bbox.max.x, bbox.min.y, bbox.min.z)),
						worldToScreen(Type3(bbox.min.x, bbox.min.y, bbox.max.z)),
						worldToScreen(Type3(bbox.max.x, bbox.min.y, bbox.max.z)),
						worldToScreen(Type3(bbox.min.x, bbox.max.y, bbox.min.z))};

	return Rect<Type2>(corners[1].x, corners[3].y, corners[0].x, corners[2].y);
}

inline float2 worldToScreen(const float2 &pos) { return worldToScreen(float3(pos.x, 0.0f, pos.y)); }

// Plane touches the sphere which encloses the box
vector<float3> genPointsOnPlane(const FBox &box, const float3 &dir, int density, bool outside);
vector<float3> genPoints(const FBox &bbox, int density);

void findPerpendicular(const float3 &v1, float3 &v2, float3 &v3);
float3 perturbVector(const float3 &vec, float rand1, float rand2, float strength);

namespace fwk {
namespace gfx {}
class XMLNode;
class XMLDocument;
class Texture;
}

namespace ui {
class Window;
class Button;
class ImageButton;
class FileDialog;
class MessageBox;
class EditBox;
typedef Ptr<Window> PWindow;
typedef Ptr<Button> PButton;
typedef Ptr<ImageButton> PImageButton;
typedef Ptr<FileDialog> PFileDialog;
typedef Ptr<MessageBox> PMessageBox;
};

class SceneRenderer;

namespace game {
class Tile;
class Sprite;
class Entity;
class TileMap;
class EntityMap;
class World;
class Actor;
class Character;
class Inventory;
class ActorInventory;
class PlayableCharacter;
class GameMode;
class GameModeServer;
class GameModeClient;
class WorldViewer;
class EntityRef;
class PCController;
class Order;

struct Item;
struct Weapon;

typedef Ptr<Sprite> PSprite;
typedef Ptr<World> PWorld;
typedef Ptr<Character> PCharacter;
typedef Ptr<PlayableCharacter> PPlayableCharacter;
typedef Ptr<PCController> PPCController;

typedef unique_ptr<GameMode> PGameMode;
typedef unique_ptr<Entity> PEntity;

typedef ClonablePtr<Order> POrder;
}

namespace net {
class Client;
class Server;
class TempPacket;
typedef unique_ptr<Client> PClient;
typedef unique_ptr<Server> PServer;
}

namespace audio {
class Playback;
typedef Ptr<Playback> PPlayback;
}

namespace io {
class Controller;
typedef unique_ptr<Controller> PController;
}

struct TupleParser;

#endif
