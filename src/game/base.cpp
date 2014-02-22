/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/base.h"
#include "audio/device.h"
#include "sys/xml.h"
#include <cstring>

#include "game/item.h"
#include "game/weapon.h"
#include "game/armour.h"
#include "game/projectile.h"

namespace game {

	DEFINE_ENUM(WeaponClassId,
		"unarmed",
		"club",
		"heavy",
		"knife",
		"minigun",
		"pistol",
		"rifle",
		"rocket",
		"smg",
		"spear"
	)

	DEFINE_ENUM(ArmourClassId,
		"none",
		"leather",
		"metal",
		"environmental",
		"power"
	)

	DEFINE_ENUM(ActorTypeId,
		"male",
		"female",
		"ghoul",
		"vault_male",
		"vault_female",
		"mutant",

		"rad_scorpion",
		"giant_rat",
		"wolf",
		"brahmin",
		"mdc",
		"sdc"
	)

	DEFINE_ENUM(ProjectileTypeId,
		"bullet",
		"plasma",
		"electric",
		"laser",
		"rocket"
	)

	DEFINE_ENUM(DeathTypeId,
		"normal",
		"big_hole",
		"cut_in_half",
		"electrify",
		"explode",
		"fire",
		"melt",
		"riddled"
	)

	DEFINE_ENUM(EntityId,
		"container",
		"door",
		"actor",
		"item",
		"projectile",
		"impact"
	)

	DEFINE_ENUM(TileId,
		"wall",
		"floor",
		"object",
		"stairs",
		"roof",
		"unknown"
	)

	DEFINE_ENUM(SurfaceId,
		"stone",
		"gravel",
		"metal",
		"wood",
		"water",
		"snow",
		"unknown"
	)

	DEFINE_ENUM(DoorTypeId,
		"rotating",
		"sliding",
		"rotating_in",
		"rotating_out"
	)

	DEFINE_ENUM(StanceId,
		"standing",
		"crouching",
		"prone"
	)

	DEFINE_ENUM(AttackMode,
		"single",
		"burst",
		"thrust",
		"slash",
		"throw",
		"punch",
		"kick",
		
		"default"
	)

	namespace AttackModeFlags {
		uint fromString(const char *string) {
			return ::toFlags(string, AttackMode::s_strings, AttackMode::count - 1, 1);
		}

		AttackMode::Type getFirst(uint flags) {
			for(int n = 0; n < AttackMode::count - 1; n++)
				if(flags & (1 << n))
					return (AttackMode::Type)n;
			return AttackMode::undefined;
		}
	};
	
	SoundId::SoundId(const char *sound_name) :m_id(audio::findSound(sound_name)) { }

	void loadPools() {
		XMLDocument tables;
		Loader("data/tables.fods") >> tables;

		WeaponDesc::load(tables, "weapons");
		AmmoDesc::load(tables, "ammo");
		ArmourDesc::load(tables, "armours");
		OtherItemDesc::load(tables, "otheritems");
		ProjectileDesc::load(tables, "projectiles");
		ImpactDesc::load(tables, "impacts");

		WeaponDesc::connectRefs();
		AmmoDesc::connectRefs();
		ArmourDesc::connectRefs();
		OtherItemDesc::connectRefs();
		ProjectileDesc::connectRefs();
		ImpactDesc::connectRefs();
	}

}
