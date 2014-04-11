#include "game/armour.h"

namespace game
{

	ArmourProto::ArmourProto(const TupleParser &parser) :ProtoImpl(parser) {
		damage_resistance = toFloat(parser("damage_resistance"));
		melee_mod = toFloat(parser("melee_mod"));
		class_id = ArmourClass::fromString(parser("class_id"));
		sound_prefix = parser("sound_prefix");
	}

}
