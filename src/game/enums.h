/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ENUMS_H
#define GAME_ENUMS_H

#include "base.h"

namespace game {

	DECLARE_ENUM(WeaponClassId,
		unarmed,
		club,
		heavy,
		knife,
		minigun,
		pistol,
		rifle,
		rocket,
		smg,
		spear
	)

	DECLARE_ENUM(ArmourClassId,
		none,
		leather,
		metal,
		environmental,
		power
	)

	DECLARE_ENUM(ItemTypeId,
		invalid = -1,
		weapon = 0,
		armour,
		ammo,
		other
	)

	DECLARE_ENUM(InventorySlotId,
		invalid = -1,
		weapon = 0,
		armour,
		ammo
	)

	DECLARE_ENUM(ActorTypeId,
		male,
		female,
		ghoul,
		vault_male,
		vault_female,
		mutant,

		rad_scorpion,
		giant_rat,
		wolf,
		brahmin,
		mdc,
		sdc
	)

	DECLARE_ENUM(ProjectileTypeId,
		bullet,
		plasma,
		electric,
		laser,
		rocket
	)

	DECLARE_ENUM(DeathTypeId,
		normal,
		big_hole,
		cut_in_half,
		electrify,
		explode,
		fire,
		melt,
		riddled
	)

	DECLARE_ENUM(EntityId,
		container,
		door,
		actor,
		item,
		projectile,
		impact
	)

	DECLARE_ENUM(TileId,
		wall,
		floor,
		object,
		stairs,
		roof,
		unknown
	)

	DECLARE_ENUM(SurfaceId,
		stone,
		gravel,
		metal,
		wood,
		water,
		snow,
		unknown
	);

	DECLARE_ENUM(DoorTypeId,
		rotating,
		sliding,
		rotating_in,
		rotating_out
	)

	DECLARE_ENUM(StanceId,
		standing,
		crouching,
		prone
	);

	inline constexpr int tileIdToFlag(TileId::Type id) { return 1 << (16 + id); }

	//TODO: better name
	enum ColliderFlags {
		collider_none			= 0x0000,

		collider_static			= 0x0002, // updates NavigationMap when its being fully recomputed
		collider_dynamic_nv		= 0x0004, // updates NavigationMap every frame
		collider_dynamic		= 0x0008, // does not update NavigationMap

		collider_item			= 0x0100,
		collider_projectile		= 0x0200,

		collider_entities		= 0x00ffff,

		collider_tile_walls		= tileIdToFlag(TileId::wall),
		collider_tile_floors	= tileIdToFlag(TileId::floor),
		collider_tile_objects	= tileIdToFlag(TileId::object),
		collider_tile_stairs	= tileIdToFlag(TileId::stairs),
		collider_tile_roofs		= tileIdToFlag(TileId::roof),
		collider_tiles			= 0xff0000,

		collider_solid			= 0xff00ff,
		collider_all			= 0xffffff,
	};
	
	enum FunctionalFlags {
		visibility_flag			= 0x80000000,
	};
	
	inline ColliderFlags operator|(ColliderFlags a, ColliderFlags b) { return (ColliderFlags)((int)a | (int)b); }

	enum class FiringMode {
		single,
		burst,
	};



}

#endif
