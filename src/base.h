/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef BASE_H
#define BASE_H

#include "fwk.h"

namespace fwk {

template <class T>
void loadFromStream(Maybe<T> &obj, Stream &sr) {
	char exists;
	sr >> exists;
	if(exists) {
		T tmp;
		sr >> tmp;
		obj = tmp;
	}
	else {
		obj = {};
	}
}

template <class T>
void saveToStream(const Maybe<T> &obj, Stream &sr) {
	sr << char(obj? 1 : 0);
	if(obj)
		sr << *obj;
}

// TODO: validation of data from files / net / etc.
// TODO: serialization of enum should automatically verify the enum
template <class T> auto validEnum(T value) -> typename std::enable_if<IsEnum<T>::value, bool>::type {
	return (int)value >= 0 && (int)value < count<T>();
}

inline int abs(int value) { return value < 0? -value : value; }

}

using namespace fwk;

inline int2 round(const float2 &v) { return int2(v.x + 0.5f, v.y + 0.5f); }
inline int3 round(const float3 &v) { return int3(v.x + 0.5f, v.y + 0.5f, v.z + 0.5f); }

inline int2 ceil(const float2 &v) {
	return int2(v.x + (1.0f - constant::epsilon), v.y + (1.0f - constant::epsilon));
}
inline int3 ceil(const float3 &v) {
	return int3(v.x + (1.0f - constant::epsilon), v.y + (1.0f - constant::epsilon),
				v.z + (1.0f - constant::epsilon));
}
void encodeInt(Stream &sr, int value);
int decodeInt(Stream &sr);

uint toFlags(const char *input, CRange<const char*> strings, uint first_flag);

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

template <class T> class ClonablePtr : public unique_ptr<T> {
  public:
	ClonablePtr(const ClonablePtr &rhs) : unique_ptr<T>(rhs ? rhs->clone() : nullptr) {
		static_assert(std::is_same<decltype(&T::clone), T *(T::*)() const>::value, "");
	}
	ClonablePtr(ClonablePtr &&rhs) : unique_ptr<T>(std::move(rhs)) {}
	ClonablePtr(T *ptr) : unique_ptr<T>(ptr) {}
	ClonablePtr() {}

	explicit operator bool() const { return unique_ptr<T>::operator bool(); }
	bool isValid() const { return unique_ptr<T>::operator bool(); }

	void operator=(ClonablePtr &&rhs) { unique_ptr<T>::operator=(std::move(rhs)); }
	void operator=(const ClonablePtr &rhs) {
		if(&rhs == this)
			return;
		T *clone = rhs ? rhs->clone() : nullptr;
		unique_ptr<T>::reset(clone);
	}
};

const float2 worldToScreen(const float3 &pos);
const int2 worldToScreen(const int3 &pos);

const float2 screenToWorld(const float2 &pos);
const int2 screenToWorld(const int2 &pos);

const Ray screenRay(const int2 &screen_pos);
const float3 project(const float3 &point, const Plane &plane);

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

struct Interval {
	Interval(float value) :min(value), max(value) { }
	Interval(float min, float max) :min(min), max(max) { }
	Interval() { }

	Interval operator+(const Interval &rhs) const { return Interval(min + rhs.min, max + rhs.max); }
	Interval operator-(const Interval &rhs) const { return Interval(min - rhs.max, max - rhs.min); }
	Interval operator*(const Interval &rhs) const;
	Interval operator*(float) const;
	Interval operator/(float val) const { return operator*(1.0f / val); }

	bool isValid() const { return min <= max; }
	
	float min, max;
};

Interval abs(const Interval&);
Interval floor(const Interval&);
Interval min(const Interval&, const Interval&);
Interval max(const Interval&, const Interval&);

float intersection(const Interval idir[3], const Interval origin[3], const Box<float3> &box);

bool isInsideFrustum(const float3 &eye_pos, const float3 &eye_dir, float min_dot, const Box<float3> &box);

template <class Type2>
inline const Rect<Type2> inset(Rect<Type2> rect, const Type2 &tl, const Type2 &br) {
	return Rect<Type2>(rect.min + tl, rect.max - br);
}
	
template <class Type2>
inline const Rect<Type2> inset(Rect<Type2> rect, const Type2 &inset) {
	return Rect<Type2>(rect.min + inset, rect.max - inset);
}

namespace fwk {
namespace gfx {}
class XMLNode;
class XMLDocument;
class Texture;
}

using PFont = unique_ptr<Font>;

namespace ui {
class Window;
class Button;
class ImageButton;
class FileDialog;
class MessageBox;
class EditBox;
using PWindow = shared_ptr<Window>;
using PButton = shared_ptr<Button>;
using PImageButton = shared_ptr<ImageButton>;
using PFileDialog = shared_ptr<FileDialog>;
using PMessageBox = shared_ptr<MessageBox>;
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

using PSprite = shared_ptr<Sprite>;
using PWorld = shared_ptr<World>;
using PCharacter = shared_ptr<Character>;
using PPlayableCharacter = shared_ptr<PlayableCharacter>;
using PPCController = shared_ptr<PCController>;

using PGameMode = unique_ptr<GameMode>;
using PEntity = unique_ptr<Entity>;

using POrder = ::ClonablePtr<Order>;
}

namespace net {
class Client;
class Server;
class TempPacket;
using PClient = unique_ptr<Client>;
using PServer = unique_ptr<Server>;
}

namespace audio {
class Playback;
using PPlayback = shared_ptr<Playback>;
}

namespace io {
class Controller;
using PController = unique_ptr<Controller>;
}

struct TupleParser;

namespace res {
ResourceManager<DTexture> &guiTextures();
ResourceManager<DTexture> &textures();
ResourceManager<game::Tile> &tiles();
PFont getFont(const string &name);
}


#endif
