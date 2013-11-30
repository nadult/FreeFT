/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/enums.h"
#include "base.h"
#include <cstring>

static int fromString(const char *str, const char **strings, int count) __attribute((noinline));
static int fromString(const char *str, const char **strings, int count) {
	DASSERT(str);
	for(int n = 0; n < count; n++)
		if(strcmp(str, strings[n]) == 0)
			return n;
	
	ASSERT(0);
	return -1;
}

#define DEFINE_STRINGS(type, ...) \
		namespace type { \
			static const char *s_strings[count] = { __VA_ARGS__ }; \
			const char *toString(Type value) { \
				DASSERT(value >= 0 && value < count); \
				return s_strings[value]; \
			} \
			Type fromString(const char *str) { \
				return (Type)::fromString(str, s_strings, count); \
			} }

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
		"floor",
		"wall",
		"roof",
		"object",
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

#undef DEFINE_STRINGS
}
