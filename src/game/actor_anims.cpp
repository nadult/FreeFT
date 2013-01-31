#/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "game/actor.h"
#include "game/world.h"
#include "game/sprite.h"
#include <cmath>
#include <cstdio>

namespace game {

	namespace ActionId {

		bool isNormal(Type action) {
			return action >= first_normal && action < first_simple;
		}

		bool isSimple(Type action) {
			return action >= first_simple && action < first_special;
		}

		bool isSpecial(Type action) {
			return action >= first_special && action < count;
		}

	};

	static const char *s_weapon_names[WeaponClassId::count] = {
		"",
		"Club",
		"Heavy",
		"Knife",
		"Minigun",
		"Pistol",
		"Rifle",
		"Rocket",
		"SMG",
		"Spear",
	};

	static const char *s_attack_names[WeaponClassId::count * 2] = {
		"UnarmedOne",
		"UnarmedTwo",

		"ClubSwing",
		nullptr,

		"HeavySingle",
		"HeavyBurst",

		"KnifeSlash",
		nullptr,

		"MinigunBurst",
		nullptr,

		"PistolSingle",
		nullptr,

		"RifleSingle", 
		"RifleBurst",

		"RocketSingle",
		nullptr,

		"SMGSingle",
		"SMGBurst",

		"SpearThrow",
		nullptr,
	};

	static const char *s_stance_names[StanceId::count] = {
		"Stand",
		"Crouch",
		"Prone"
	};

	static const char *s_normal_names[ActionId::first_simple - ActionId::first_normal] = {
		"",
		"Walk",
		"Attack",
		"Attack",
	};

	static const char *s_simple_names[ActionId::first_special - ActionId::first_simple][StanceId::count] = {
		// standing, 	crouching,	prone,
		{ "Run",		nullptr,	nullptr },		// running

		{ nullptr,		"Stand", 	"Crouch" },		// stance up
		{ "Crouch",		"Prone", 	nullptr },		// stance down


		{ "Pickup", 	"Pickup", 	"Pickup" },		// pickup
		{ "Magichigh",	"Magic",	"Magic" },		// magic 1
		{ "Magiclow", 	"Magic", 	"Magic" }		// magic 2
	};

	static const char *s_death_names[DeathTypeId::count] = {
		"",
		"Bighole",
		"Cutinhalf",
		"Electrify",
		"Explode",
		"Fire",
		"Melt",
		"Riddled"
	};

	ActorAnims::ActorAnims(PSprite sprite) {
		DASSERT(sprite);

		for(int d = 0; d < DeathTypeId::count; d++) {
			const string &name = deathAnimName((DeathTypeId::Type)d);
			m_death_ids[d] = name.empty()? -1 : sprite->findSequence(name.c_str());
		}

		for(int a = 0; a < ActionId::count; a++)
			for(int s = 0; s < StanceId::count; s++) {
				if(ActionId::isNormal((ActionId::Type)a)) {
					for(int w = 0; w < WeaponClassId::count; w++) {
						const string &name = animName((ActionId::Type)a, (StanceId::Type)s, (WeaponClassId::Type)w);
						m_normal_ids[a][s][w] = name.empty()? -1 : sprite->findSequence(name.c_str());
					}
				}
				else if(ActionId::isSimple((ActionId::Type)a)) {
					const string &name = simpleAnimName((ActionId::Type)a, (StanceId::Type)s);
					m_simple_ids[a - ActionId::first_simple][s] = name.empty()? -1 : sprite->findSequence(name.c_str());
				}
			}
		
		setFallbackAnims();
	}

	const string ActorAnims::deathAnimName(DeathTypeId::Type id) const {
		char text[128];
		snprintf(text, sizeof(text), "Death%s", s_death_names[id]);
		return text;
	}

	const string ActorAnims::simpleAnimName(ActionId::Type action, StanceId::Type stance) const {
		DASSERT(ActionId::isSimple(action));
		const char *name = s_simple_names[action - ActionId::first_simple][stance];
		if(!name)
			return "";

		char text[128];
		snprintf(text, sizeof(text), "%s%s", s_stance_names[stance], name);
		return text;
	}

	const string ActorAnims::animName(ActionId::Type action, StanceId::Type stance, WeaponClassId::Type weapon) const {
		DASSERT(!ActionId::isSpecial(action));
		if(ActionId::isSimple(action))
			return simpleAnimName(action, stance);

		char text[128] = "";
		if(action == ActionId::attack1 || action == ActionId::attack2) {
			const char *attack_name = s_attack_names[weapon * 2 + (action == ActionId::attack1? 0 : 1)];
			if(attack_name)
				snprintf(text, sizeof(text), "%s%s%s", s_stance_names[stance], s_normal_names[action], attack_name);
		}
		else 
			snprintf(text, sizeof(text), "%s%s%s", s_stance_names[stance], s_normal_names[action], s_weapon_names[weapon]);

		return text;
	}
	
	int ActorAnims::deathAnimId(DeathTypeId::Type id) const {
		return m_death_ids[id];
	}

	int ActorAnims::simpleAnimId(ActionId::Type action, StanceId::Type stance) const {
		DASSERT(ActionId::isSimple(action));
		return m_simple_ids[action - ActionId::first_simple][stance];
	}

	int ActorAnims::animId(ActionId::Type action, StanceId::Type stance, WeaponClassId::Type weapon) const {
		DASSERT(!ActionId::isSpecial(action));
		return ActionId::isSimple(action)? simpleAnimId(action, stance) : m_normal_ids[action][stance][weapon];
	}

	bool ActorAnims::canChangeStance() const {
		for(int s = 0; s < StanceId::count; s++) {
			if(animId(ActionId::idle, (StanceId::Type)s, WeaponClassId::unarmed) == -1)
				return false;
			if(animId(ActionId::walking, (StanceId::Type)s, WeaponClassId::unarmed) == -1)
				return false;
		}

		if(	simpleAnimId(ActionId::stance_up, StanceId::prone) == -1 ||
			simpleAnimId(ActionId::stance_up, StanceId::crouching) == -1 ||
			simpleAnimId(ActionId::stance_down, StanceId::crouching) == -1 ||
			simpleAnimId(ActionId::stance_down, StanceId::standing) == -1)
			return false;

		return true;
	}

	void ActorAnims::setFallbackAnims() {
		for(int d = 0; d < DeathTypeId::count; d++)
			if(m_death_ids[d] == -1)
				m_death_ids[d] = m_death_ids[DeathTypeId::normal];

		for(int w = 0; w < WeaponClassId::count; w++)
			for(int s = 0; s < StanceId::count; s++)
				if(m_normal_ids[ActionId::walking][s][w] == -1 && m_normal_ids[ActionId::idle][s][w] != -1)
					m_normal_ids[ActionId::walking][s][w] = m_normal_ids[ActionId::walking][s][WeaponClassId::unarmed];
	}

	// sets seq_id, frame_id and seq_name
	void Actor::animate(ActionId::Type action_id) {
		int seq_id = m_anims.animId(action_id, m_stance_id, m_weapon_class_id);
		if(seq_id == -1) {
			//TODO: better error handling of missing sequences
			printf("Sequence: %s not found!\n",
					m_anims.animName(action_id, m_stance_id, m_weapon_class_id).c_str());
			ASSERT(seq_id != -1);
		}

		playSequence(seq_id);
		m_action_id = action_id;
	}

	void Actor::animateDeath(DeathTypeId::Type death_id) {
		playSequence(m_anims.deathAnimId(death_id));
		m_action_id = ActionId::death;
	}


}
