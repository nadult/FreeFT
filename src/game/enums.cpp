/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/enums.h"
#include "base.h"
#include <cstring>


namespace game {

	DEFINE_STRINGS( WeaponClassId,
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

	DEFINE_STRINGS( ArmourClassId,
		"none",
		"leather",
		"metal",
		"environmental",
		"power"
	)

	DEFINE_STRINGS( InventorySlotId,
		"weapon",
		"armour",
		"ammo"
	)

	DEFINE_STRINGS( ItemTypeId,
		"weapon",
		"armour",
		"ammo",
		"other"
	)

	DEFINE_STRINGS( ActorTypeId,
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

	DEFINE_STRINGS( ProjectileTypeId,
		"bullet",
		"plasma",
		"electric",
		"laser",
		"rocket"
	)

	DEFINE_STRINGS(DeathTypeId,
		"normal",
		"big_hole",
		"cut_in_half",
		"electrify",
		"explode",
		"fire",
		"melt",
		"riddled"
	)

	DEFINE_STRINGS(EntityId,
		"container",
		"door",
		"actor",
		"item",
		"projectile",
		"impact"
	)

	DEFINE_STRINGS(TileId,
		"wall",
		"floor",
		"object",
		"stairs",
		"roof",
		"unknown"
	)

	DEFINE_STRINGS(SurfaceId,
		"stone",
		"gravel",
		"metal",
		"wood",
		"water",
		"snow",
		"unknown"
	)

	DEFINE_STRINGS(DoorTypeId,
		"rotating",
		"sliding",
		"rotating_in",
		"rotating_out"
	)

	DEFINE_STRINGS(StanceId,
		"standing",
		"crouching",
		"prone"
	)

}
