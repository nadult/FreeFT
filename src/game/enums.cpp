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
		"vault_female"
	)

	DEFINE_STRINGS( ProjectileTypeId,
		"bullet",
		"plasma",
		"laser",
		"rocket"
	)


#undef DEFINE_STRINGS
}
