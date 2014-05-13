#include "game/ammo.h"

namespace game
{
	AmmoProto::AmmoProto(const TupleParser &parser) :ProtoImpl(parser) {
		damage_mod = toFloat(parser("damage_mod"));
		class_id = parser("class_id");
	}

}
