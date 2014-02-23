#include "game/armour.h"

namespace game
{

	ArmourProto::ArmourProto(const TupleParser &parser) :ProtoImpl(parser) {
		damage_resistance = toFloat(parser("damage_resistance"));
		class_id = ArmourClassId::fromString(parser("class_id"));
	}

}
