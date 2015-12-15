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
		"turret",
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

	DEFINE_ENUM(OrderTypeId,
		"idle",
		"look_at",
		"move",
		"track",
		"attack",
		"change_stance",
		"interact",
		"drop_item",
		"equip_item",
		"unequip_item",
		"transfer_item",
		"get_hit",
		"die"
	)

	DEFINE_ENUM(GameModeId,
		"single_player",
		"death_match",
		"trench_war",
		"hunter"
	)

	DEFINE_ENUM(SoundType,
		"normal",
		"explosion",
		"shooting"
	)

	DEFINE_ENUM(HealthStatus,
		"unhurt",
		"barely wounded",
		"wounded",
		"seriously wounded",
		"near death",
		"dead"
	);

	namespace HealthStatus {
		Type fromHPPercentage(float health) {
			return
				health > 0.99f? unhurt :
				health > 0.70f? barely_wounded :
				health > 0.50f? wounded :
				health > 0.20f? seriously_wounded :
				health > 0.0f ? near_death : dead;
		}
	}

	namespace AttackModeFlags {
		uint fromString(const char *string) {
			return toFlags(string, AttackMode::s_strings, AttackMode::count, 1);
		}

		AttackMode::Type getFirst(uint flags) {
			for(int n = 0; n < AttackMode::count; n++)
				if(flags & (1 << n))
					return (AttackMode::Type)n;
			return AttackMode::undefined;
		}
	};
	
	SoundId::SoundId(const char *sound_name, int offset) {
		audio::SoundIndex index = audio::findSound(sound_name);
		m_id = (int)index + (offset < 0 || offset > index.variation_count? 0 : offset);
	}
}
