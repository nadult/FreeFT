#include "game/armour.h"

namespace game
{

	ArmourDesc::ArmourDesc(const TupleParser &parser) :ItemDesc(parser) {
		damage_resistance = toFloat(parser("damage_resistance"));
		class_id = ArmourClassId::fromString(parser("class_id"));
	}

}
