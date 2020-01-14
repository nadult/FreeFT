/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
*/

#include "game/actor.h"
#include "game/world.h"
#include "game/sprite.h"

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

	static const EnumMap<Stance, const char*> s_simple_names[Action::_special - Action::_simple] = {
		// prone, 			crouch,				stand,
		{{ "fallback",		"fallback",			"fallback" }},
		{{ "fallenback",	"fallenback",		"fallenback" }},
		{{ "fallforward",	"fallforward",		"fallforward" }},
		{{ "fallenforward",	"fallenforward",	"fallenforward" }},
		{{ "getupback",		"getupback",		"getupback" }},
		{{ "getupforward",	"getupforward", 	"getupforward" }},
		{{ "recoil",		"recoil",			"recoil" }},
		{{ "dodgeone",		"dodgeone",			"dodgeone" }},
		{{ "dodgetwo",		"dodgetwo", 		"dodgetwo" }},
		{{ "fidget",		"fidget",			"fidget" }},
		{{ "magic",			"magic",			"magichigh" }},
		{{ "magic", 		"magic", 			"magiclow" }},
		{{ nullptr,			nullptr,			"run" }},
		{{ "crouch",		"stand",		 	nullptr }},
		{{ nullptr,			"prone", 			"crouch" }},
		{{ "pickup", 		"pickup", 			"pickup" }}
	};

	ActorArmourProto::ActorArmourProto(const TupleParser &parser, bool is_actor)
		:ProtoImpl(parser), m_is_actor(is_actor) {
		ASSERT(!is_dummy);

		if(m_is_actor) {
			armour = "_dummy_armour";
		}
		else {
			actor = parser("actor_id");
			armour = parser("armour_id");
		}

		initAnims();

		for(auto w : all<WeaponClass>()) {
			m_can_equip_weapon[w] = false;

			for(auto s : all<Stance>()) {
				m_can_use_weapon[w][s] = false;

				for(auto a : all<AttackMode>())
					if(attackAnimId(a, s, w) != -1) {
						m_can_equip_weapon[w] = true;
						m_can_use_weapon[w][s] = true;
					}
				if(	animId(Action::idle, (Stance)s, (WeaponClass)w) == -1 ||
					animId(Action::walk, (Stance)s, (WeaponClass)w) == -1)
					m_can_use_weapon[w][s] = false;
			}
		}

		m_can_change_stance = true;
		for(auto s : all<Stance>()) {
			if(animId(Action::idle, (Stance)s, WeaponClass::unarmed) == -1)
				m_can_change_stance = false;
			if(animId(Action::walk, (Stance)s, WeaponClass::unarmed) == -1)
				m_can_change_stance = false;
		}
		if(	simpleAnimId(Action::stance_up, Stance::prone) == -1 ||
			simpleAnimId(Action::stance_up, Stance::crouch) == -1 ||
			simpleAnimId(Action::stance_down, Stance::crouch) == -1 ||
			simpleAnimId(Action::stance_down, Stance::stand) == -1)
			m_can_change_stance = false;
	}
		
	bool ActorArmourProto::canEquipWeapon(WeaponClass weapon) const {
		return m_can_equip_weapon[weapon];
	}

	bool ActorArmourProto::canUseWeapon(WeaponClass weapon, Stance stance) const {
		return m_can_use_weapon[weapon][stance];
	}

	bool ActorArmourProto::canChangeStance() const {
		return m_can_change_stance;
	}

	void ActorArmourProto::link() {
		if(m_is_actor)
			actor = index();
		else
			actor.link();
		ASSERT(actor);
		armour.link();

		for(auto st : all<Stance>())
			for(auto su : all<SurfaceId>()) {
				char name[256];
				snprintf(name, sizeof(name), "%s%s%s%s",
						st == Stance::prone? "prone" : "stand", actor->is_heavy? "heavy" : "normal",
						armour->sound_prefix.c_str(), toString(su));
				step_sounds[st][su] = SoundId(name);
			}

		{
			ArmourClass class_id = armour? armour->class_id : ArmourClass::none;
			const char *postfix =
				class_id == ArmourClass::leather? "leath" :
				class_id == ArmourClass::metal? "metal" :
				class_id == ArmourClass::power || class_id == ArmourClass::environmental? "pow" : "cloth";
			char fall_sound_name[256];
			snprintf(fall_sound_name, sizeof(fall_sound_name), "bfall_%s_%s", actor->is_heavy? "heavy" : "normal", postfix);
			fall_sound = fall_sound_name;
		}
	}

	ActorProto::ActorProto(const TupleParser &parser) :ProtoImpl(parser, true) {
		punch_weapon = parser("punch_weapon");
		kick_weapon = parser("kick_weapon");
		sound_prefix = parser("sound_prefix");
		is_heavy = parser.get<bool>("is_heavy");
		is_alive = parser.get<bool>("is_alive");

		float4 speed_vec = parser.get<float4>("speeds");
		speeds[0] = speed_vec.x;
		speeds[1] = speed_vec.y;
		speeds[2] = speed_vec.z;
		speeds[3] = speed_vec.w;

		hit_points = parser.get<float>("hit_points");
	}


	static const EnumMap<DeathId, const char *> s_death_names = {{
		"",
		"bighole",
		"cutinhalf",
		"electrify",
		"explode",
		"fire",
		"melt",
		"riddled"
	}};

	static const char *deathName(DeathId death) {
		return death == DeathId::normal? "death" : s_death_names[death];
	}

	void ActorProto::link() {
		ActorArmourProto::link();
		punch_weapon.link();
		kick_weapon.link();
		
		char text[256];
		for(auto d : all<DeathId>()) {
			snprintf(text, sizeof(text), "human%s", deathName(d));
			human_death_sounds[d] = SoundId(text);
		}

		snprintf(text, sizeof(text), "%srun", sound_prefix.c_str());
		run_sound = text;
		
		snprintf(text, sizeof(text), "%swalk", sound_prefix.c_str());
		walk_sound = text;

		for(int n = 0;; n++) {
			char var_name[128];
			if(n == 0)
				snprintf(var_name, sizeof(var_name), "%s", sound_prefix.c_str());
			else
				snprintf(var_name, sizeof(var_name), "%s%d", sound_prefix.c_str(), n);

			Sounds sounds;

			bool any_sound = false;

			for(auto d : all<DeathId>()) {
				snprintf(text, sizeof(text), "%s%s", var_name, deathName(d));
				sounds.death[d] = SoundId(text);
				any_sound |= sounds.death[d].isValid();
			}
		
			snprintf(text, sizeof(text), "%sgetup", var_name);
			sounds.get_up = text;
			snprintf(text, sizeof(text), "%shit", var_name);
			sounds.hit = text;

			any_sound |= sounds.get_up.isValid() | sounds.hit.isValid();

			if(!any_sound && n >= 1)
				break;
			if(any_sound)
				this->sounds.emplace_back(sounds);
		}
		if(sounds.empty())
			sounds.emplace_back(Sounds());
	}

	enum { invalid_id = 255 };

	void ActorArmourProto::initAnims() {
		ASSERT(sprite);
		ASSERT(sprite->size() <= invalid_id - 1);

		std::map<string, int> name_map;
		for(int n = 0; n < sprite->size(); n++)
			name_map.emplace(toLower((*sprite)[n].name), n);

		char text[256];

		for(auto d : all<DeathId>()) {
			snprintf(text, sizeof(text), "death%s", s_death_names[d]);
			auto it = name_map.find(text);
			m_death_idx[d] = it == name_map.end()? invalid_id : it->second;
		}

		for(auto w : all<WeaponClass>())
			for(auto a : all<AttackMode>())
				for(auto s : all<Stance>()) {
					const char *attack_mode =	a == AttackMode::punch? "one" :
												a == AttackMode::kick? "two" : toString(a);

					snprintf(text, sizeof(text), "%sattack%s%s", toString(s), toString(w), attack_mode);
					auto it = name_map.find(text);
					m_attack_idx[w][a][s] = it == name_map.end()? invalid_id : it->second;
				//	if(it != name_map.end())
				//		printf("%s: %s\n", text, m_attack_idx[w][a][s] == 255? "NOT FOUND" : "OK");
				}

		for(int a = 0; a < Action::count; a++)
			for(auto s : all<Stance>()) {
				if(Action::isNormal(a)) {
					for(auto w : all<WeaponClass>()) {
						snprintf(text, sizeof(text), "%s%s%s", toString(s), s_normal_names[a],
									w == WeaponClass::unarmed? "" : toString(w));
						auto it = name_map.find(text);
						m_normal_idx[a][s][w] = it == name_map.end()? invalid_id : it->second;
					//	printf("%s: %s\n", text, m_normal_idx[a][s][w] == 255? "NOT FOUND" : "OK");
					}
				}
				else if(Action::isSimple(a)) {
					snprintf(text, sizeof(text), "%s%s", toString(s), s_simple_names[a - Action::_simple][s]);
					auto it = name_map.find(text);
					m_simple_idx[a - Action::_simple][s] = it == name_map.end()? invalid_id : it->second;
				}
			}

		for(int c = 0; c < arraySize(m_climb_idx); c++) {
			auto it = name_map.find(s_climb_names[c]);
			m_climb_idx[c] = it == name_map.end()? invalid_id : it->second;
		}

		for(auto w : all<WeaponClass>())
			for(auto s = Stance::prone; s <= Stance::crouch; s = next(s))
			if(m_normal_idx[Action::walk][s][w] == invalid_id)
				m_normal_idx[Action::walk][s][w] = m_normal_idx[Action::walk][s][WeaponClass::unarmed];

		ASSERT(animId(Action::idle, Stance::stand, WeaponClass::unarmed) != -1);
	}

	int ActorArmourProto::climbAnimId(Action::Type action) {
		DASSERT(action >= Action::climb && action <= Action::climb_down);

		int anim_id = m_climb_idx[action - Action::climb];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::attackAnimId(AttackMode mode, Stance stance, WeaponClass weapon) const {
		int anim_id = m_attack_idx[weapon][mode][stance];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::deathAnimId(DeathId id) const {
		int anim_id = m_death_idx[id];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::simpleAnimId(Action::Type action, Stance stance) const {
		DASSERT(Action::isSimple(action));

		int anim_id = m_simple_idx[action - Action::_simple][stance];
		return anim_id == invalid_id? -1 : anim_id;
	}

	int ActorArmourProto::animId(Action::Type action, Stance stance, WeaponClass weapon) const {
		DASSERT(!Action::isSpecial(action));

		int anim_id = Action::isSimple(action)? simpleAnimId(action, stance) : m_normal_idx[action][stance][weapon];
		return anim_id == invalid_id? -1 : anim_id;
	}

}
