/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/base.h"
#include "audio/device.h"
#include <cstring>

namespace game {

	DEFINE_ENUM(WeaponClass,
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

	DEFINE_ENUM(ArmourClass,
		"none",
		"leather",
		"metal",
		"environmental",
		"power"
	)

	DEFINE_ENUM(DamageType,
		"bludgeoning",
		"slashing",
		"piercing",
		"bullet",
		"fire",
		"plasma",
		"laser",
		"electric",
		"explosive",
	)

	DEFINE_ENUM(DeathId,
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
		"impact",
		"trigger"
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

	DEFINE_ENUM(Stance,
		"prone",
		"crouch",
		"stand"
	)

	DEFINE_ENUM(SentryMode,
		"passive",
		"aggresive",
		"defensive"
	)

	DEFINE_ENUM(AttackMode,
		"single",
		"burst",
		"thrust",
		"slash",
		"swing",
		"throw",
		"punch",
		"kick"
	)

	DEFINE_ENUM(GameModeId,
		"death_match",
		"trench_war"
	)

	DEFINE_ENUM(SoundType,
		"normal",
		"explosion",
		"shooting"
	)

	namespace AttackModeFlags {
		uint fromString(const char *string) {
			return ::toFlags(string, AttackMode::s_strings, AttackMode::count, 1);
		}

		AttackMode::Type getFirst(uint flags) {
			for(int n = 0; n < AttackMode::count; n++)
				if(flags & (1 << n))
					return (AttackMode::Type)n;
			return AttackMode::undefined;
		}
	};
	
	SoundId::SoundId(const char *sound_name) :m_id(audio::findSound(sound_name)) { }
}
