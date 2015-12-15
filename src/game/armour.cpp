#include "game/armour.h"

namespace game
{

	ArmourProto::ArmourProto(const TupleParser &parser) :ProtoImpl(parser) {
		damage_resistance = parser.get<float>("damage_resistance");
		melee_mod = parser.get<float>("melee_mod");
		class_id = ArmourClass::fromString(parser("class_id"));
		sound_prefix = parser("sound_prefix");
	}

	const string Armour::paramDesc() const {
		TextFormatter out;
		float resistance = proto().damage_resistance;
		float melee_mod = proto().melee_mod;

		out("Resistance: %c%.0f%%\n", resistance >= 0.0f? '+' : '-', fabs(resistance * 100.0f));
		if(melee_mod != 1.0f)
			out("Strength mod: %.0f%%\n", melee_mod * 100.0f);
		return string(out.text());
	}

}
