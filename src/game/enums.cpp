/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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

	DEFINE_STRINGS(EntityId,
		"container",
		"door",
		"actor",
		"item",
		"projectile",
		"impact"
	);

	DEFINE_STRINGS(TileId,
		"floor",
		"wall",
		"roof",
		"object",
		"unknown"
	);

#undef DEFINE_STRINGS
}
