#include "game/ammo.h"

namespace game
{
	AmmoProto::AmmoProto(const TupleParser &parser) :ProtoImpl(parser) {
		damage_mod = toFloat(parser("damage_mod"));
		class_id = parser("class_id");
	}

	const string Ammo::paramDesc() const {
		TextFormatter out;
		float damage_mod = proto().damage_mod - 1.0f;
		if(damage_mod != 0.0f)
			out("Damage mod: %c%.0f%%\n", damage_mod >= 0.0f? '+' : '-', fabs(damage_mod * 100.0f));
		return string(out.text());
	}


}
