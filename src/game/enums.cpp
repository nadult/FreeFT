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

namespace game {

#define STRING_FUNCTIONS() \
		const char *toString(Type value) { \
			DASSERT(value >= 0 && value < count); \
			return s_strings[value]; \
		} \
		Type fromString(const char *str) { \
			return (Type)::fromString(str, s_strings, count); \
		}

	namespace ItemTypeId {
		static const char *s_strings[count] = {
			"weapon",
			"ammo",
			"armour",
			"other",
		};

		STRING_FUNCTIONS();
	};

	namespace WeaponClassId {
		static const char *s_strings[count] = {
			"unarmed",
			"club",
			"heavy",
			"knife",
			"minigun",
			"pistol",
			"rifle",
			"rocket",
			"smg",
			"spear",
		};

		STRING_FUNCTIONS();
	};

	namespace ProjectileTypeId {
		static const char *s_strings[count] = {
			"bullet",
			"plasma",
			"laser",
			"rocket",
		};

		STRING_FUNCTIONS();
	};


#undef STRING_FUNCTIONS
}
