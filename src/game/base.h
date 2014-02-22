/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_BASE_H
#define GAME_BASE_H

#include "../base.h"

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

	DECLARE_ENUM(StanceId,
		standing,
		crouching,
		prone
	);

	DECLARE_ENUM(AttackMode,
		single,
		burst,
		thrust,
		slash,
		throwing,
		punch,
		kick,

		undefined
	);
	
	namespace AttackMode {
		inline constexpr uint toFlags(Type t) { return t == undefined? 0 : 1 << t; }
		inline int actionId(Type t) { return t == burst || t == kick? 2 : 1; }
	};

	namespace AttackModeFlags {
		enum Type {
			single		= toFlags(AttackMode::single),
			burst		= toFlags(AttackMode::burst),
			thrust		= toFlags(AttackMode::thrust),
			slash		= toFlags(AttackMode::slash),
			throwing	= toFlags(AttackMode::throwing),
		};

		uint fromString(const char*);

		AttackMode::Type getFirst(uint flags);
	};
	

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
	
	inline constexpr ColliderFlags operator|(ColliderFlags a, ColliderFlags b)
			{ return (ColliderFlags)((int)a | (int)b); }

	class SoundId {
	public:
		SoundId() :m_id(-1) { }
		SoundId(const char *sound_name);
		operator int() const { return m_id; }

	protected:
		int m_id;
	};

	void loadPools();

}

#endif
