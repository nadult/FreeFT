/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
*/

#include "game/actor.h"
#include "game/world.h"
#include "game/sprite.h"
#include "sys/platform.h"

namespace game {

	namespace Action {

		bool isNormal(int);
		bool isSimple(int);
		bool isSpecial(int);

		bool isNormal(int id) { return id >= _normal && id < _simple; }
		bool isSimple(int id) { return id >= _simple && id < _special; }
		bool isSpecial(int id) { return id >= _special && id < count; }
		bool isValid(int id) { return id >= 0 && id < count; }

	};

	static const char *s_normal_names[Action::_simple - Action::_normal] = {
		"",
		"walk",
		"breathe"
	};

	static const char *s_climb_names[3] = {
		"climb",
		"climbup",
		"climbdown",
	};

	static const char *s_simple_names[Action::_special - Action::_simple][Stance::count] = {
		// prone, 			crouch,				stand,
		{ "fallback",		"fallback",			"fallback" },
		{ "fallenback",		"fallenback",		"fallenback" },
		{ "fallforward",	"fallforward",		"fallforward" },
		{ "fallenforward",	"fallenforward",	"fallenforward" },
		{ "getupback",		"getupback",		"getupback" },
		{ "getupforward",	"getupforward", 	"getupforward" },
		{ "recoil",			"recoil",			"recoil" },
		{ "dodgeone",		"dodgeone",			"dodgeone" },
		{ "dodgetwo",		"dodgetwo", 		"dodgetwo" },
		{ "fidget",			"fidget",			"fidget" },
		{ "magic",			"magic",			"magichigh" },
		{ "magic", 			"magic", 			"magiclow" },
		{ nullptr,			nullptr,			"run" },
		{ "crouch",			"stand",		 	nullptr },
		{ nullptr,			"prone", 			"crouch" },
		{ "pickup", 		"pickup", 			"pickup" }
	};

	static const char *s_death_names[DeathTypeId::count] = {
		"",
		"bighole",
		"cutinhalf",
		"electrify",
		"explode",
		"fire",
		"melt",
		"riddled"
	};

	ActorArmourProto::ActorArmourProto(const TupleParser &parser, bool is_actor)
		:ProtoImpl(parser), m_is_actor(is_actor) {
		ASSERT(!is_dummy);

		if(m_is_actor) {
			armour = "_dummy_armour";
			m_sound_prefix = parser("sound_prefix");
		}
		else {
			actor = parser("actor_id");
			armour = parser("armour_id");
		}

		initAnims();

		for(int w = 0; w < WeaponClass::count; w++) {
			m_can_equip_weapon[w] = false;

			for(int s = 0; s < Stance::count; s++) {
				m_can_use_weapon[w][s] = false;

				for(int a = 0; a < AttackMode::count; a++)
					if(attackAnimId((AttackMode::Type)a, (Stance::Type)s, (WeaponClass::Type)w) != -1) {
						m_can_equip_weapon[w] = true;
						m_can_use_weapon[w][s] = true;
					}
				if(	animId(Action::idle, (Stance::Type)s, (WeaponClass::Type)w) == -1 ||
					animId(Action::walk, (Stance::Type)s, (WeaponClass::Type)w) == -1)
					m_can_use_weapon[w][s] = false;
			}
		}

		m_can_change_stance = true;
		for(int s = 0; s < Stance::count; s++) {
			if(animId(Action::idle, (Stance::Type)s, WeaponClass::unarmed) == -1)
				m_can_change_stance = false;
			if(animId(Action::walk, (Stance::Type)s, WeaponClass::unarmed) == -1)
				m_can_change_stance = false;
		}
		if(	simpleAnimId(Action::stance_up, Stance::prone) == -1 ||
			simpleAnimId(Action::stance_up, Stance::crouch) == -1 ||
			simpleAnimId(Action::stance_down, Stance::crouch) == -1 ||
			simpleAnimId(Action::stance_down, Stance::stand) == -1)
			m_can_change_stance = false;
	}
		
	bool ActorArmourProto::canEquipWeapon(WeaponClass::Type weapon) const {
		DASSERT(WeaponClass::isValid(weapon));
		return m_can_equip_weapon[weapon];
	}

	bool ActorArmourProto::canUseWeapon(WeaponClass::Type weapon, Stance::Type stance) const {
		DASSERT(WeaponClass::isValid(weapon));
		DASSERT(Stance::isValid(stance));
		return m_can_use_weapon[weapon][stance];
	}

	bool ActorArmourProto::canChangeStance() const {
		return m_can_change_stance;
	}

	void ActorArmourProto::connect() {
		if(m_is_actor)
			actor = index();
		else
			actor.connect();
		armour.connect();

		for(int st = 0; st < Stance::count; st++)
			for(int su = 0; su < SurfaceId::count; su++) {
				char name[256];
				snprintf(name, sizeof(name), "%s%s%s%s",
						st == Stance::prone? "prone" : "stand", actor->is_heavy? "heavy" : "normal",
						armour->sound_prefix.c_str(), SurfaceId::toString(su));
				step_sounds[st][su] = SoundId(name);
			}

		const char *prefix = m_is_actor? m_sound_prefix.c_str() : actor->m_sound_prefix.c_str();
		for(int d = 0; d < DeathTypeId::count; d++) {
			char text[256];
			snprintf(text, sizeof(text), "%s%s", prefix, d == DeathTypeId::normal? "death" : s_death_names[d]);
			death_sounds[d] = SoundId(text);
		}
	}

	ActorProto::ActorProto(const TupleParser &parser) :ProtoImpl(parser, true) {
		is_heavy = toBool(parser("is_heavy"));
		float4 speed_vec = toFloat4(parser("speeds"));
		speeds[0] = speed_vec.x;
		speeds[1] = speed_vec.y;
		speeds[2] = speed_vec.z;
		speeds[3] = speed_vec.w;
	}

	enum { invalid_id = 255 };

	void ActorArmourProto::initAnims() {
		ASSERT(sprite);
		ASSERT(sprite->size() <= invalid_id - 1);

		std::map<string, int> name_map;
		for(int n = 0; n < sprite->size(); n++)
			name_map.emplace(toLower((*sprite)[n].name), n);

		char text[256];

		for(int d = 0; d < DeathTypeId::count; d++) {
			snprintf(text, sizeof(text), "death%s", s_death_names[d]);
			auto it = name_map.find(text);
			m_death_idx[d] = it == name_map.end()? invalid_id : it->second;
		}

		for(int w = 0; w < WeaponClass::count; w++)
			for(int a = 0; a < AttackMode::count; a++)
				for(int s = 0; s < Stance::count; s++) {
					const char *attack_mode =	a == AttackMode::punch? "one" :
												a == AttackMode::kick? "two" : AttackMode::toString(a);

					snprintf(text, sizeof(text), "%sattack%s%s", Stance::toString(s), WeaponClass::toString(w), attack_mode);
					auto it = name_map.find(text);
					m_attack_idx[w][a][s] = it == name_map.end()? invalid_id : it->second;
				//	if(it != name_map.end())
				//		printf("%s: %s\n", text, m_attack_idx[w][a][s] == 255? "NOT FOUND" : "OK");
				}

		for(int a = 0; a < Action::count; a++)
			for(int s = 0; s < Stance::count; s++) {
				if(Action::isNormal(a)) {
					for(int w = 0; w < WeaponClass::count; w++) {
						snprintf(text, sizeof(text), "%s%s%s", Stance::toString(s), s_normal_names[a],
									w == WeaponClass::unarmed? "" : WeaponClass::toString(w));
						auto it = name_map.find(text);
						m_normal_idx[a][s][w] = it == name_map.end()? invalid_id : it->second;
					//	printf("%s: %s\n", text, m_normal_idx[a][s][w] == 255? "NOT FOUND" : "OK");
					}
				}
				else if(Action::isSimple(a)) {
					snprintf(text, sizeof(text), "%s%s", Stance::toString(s), s_simple_names[a - Action::_simple][s]);
					auto it = name_map.find(text);
					m_simple_idx[a - Action::_simple][s] = it == name_map.end()? invalid_id : it->second;
				}
			}

		for(int c = 0; c < COUNTOF(m_climb_idx); c++) {
			auto it = name_map.find(s_climb_names[c]);
			m_climb_idx[c] = it == name_map.end()? invalid_id : it->second;
		}

		for(int w = 0; w < WeaponClass::count; w++)
			for(int s = Stance::prone; s <= Stance::crouch; s++)
			if(m_normal_idx[Action::walk][s][w] == invalid_id)
				m_normal_idx[Action::walk][s][w] = m_normal_idx[Action::walk][s][0];

		ASSERT(animId(Action::idle, Stance::stand, WeaponClass::unarmed) != -1);
	}

	int ActorArmourProto::climbAnimId(Action::Type action) {
		DASSERT(action >= Action::climb && action <= Action::climb_down);

		int anim_id = m_climb_idx[action - Action::climb];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::attackAnimId(AttackMode::Type mode, Stance::Type stance, WeaponClass::Type weapon) const {
		DASSERT(Stance::isValid(stance));
		DASSERT(WeaponClass::isValid(weapon));
		DASSERT(AttackMode::isValid(mode));

		int anim_id = m_attack_idx[weapon][mode][stance];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::deathAnimId(DeathTypeId::Type id) const {
		DASSERT(DeathTypeId::isValid(id));

		int anim_id = m_death_idx[id];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::simpleAnimId(Action::Type action, Stance::Type stance) const {
		DASSERT(Action::isSimple(action));
		DASSERT(Stance::isValid(stance));

		int anim_id = m_simple_idx[action - Action::_simple][stance];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::animId(Action::Type action, Stance::Type stance, WeaponClass::Type weapon) const {
		DASSERT(!Action::isSpecial(action));
		DASSERT(Stance::isValid(stance));
		DASSERT(WeaponClass::isValid(weapon));

		int anim_id = Action::isSimple(action)? simpleAnimId(action, stance) : m_normal_idx[action][stance][weapon];
		return anim_id == invalid_id? -1 : anim_id;
	}

}
