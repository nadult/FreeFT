// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "../base.h"
#include "sys/data_sheet.h"
#include <fwk/enum_flags.h>

namespace game {

	class Entity;
	class Tile;

	DEFINE_ENUM(WeaponClass, unarmed, club, heavy, knife, minigun, pistol, rifle, rocket, smg,
				spear);

	DEFINE_ENUM(ArmourClass, none, leather, metal, environmental, power);

	DEFINE_ENUM(DamageType, bludgeoning, slashing, piercing, bullet, fire, plasma, laser, electric,
				explosive);

	DEFINE_ENUM(DeathId, normal, big_hole, cut_in_half, electrify, explode, fire, melt, riddled);

	DEFINE_ENUM(EntityId, container, door, actor, turret, item, projectile, impact, trigger);

	DEFINE_ENUM(TileId, wall, floor, object, stairs, roof, unknown);

	DEFINE_ENUM(SurfaceId, stone, gravel, metal, wood, water, snow, unknown);

	DEFINE_ENUM(Stance, prone, crouch, stand);

	DEFINE_ENUM(SentryMode, passive, defensive, aggresive);

	DEFINE_ENUM(AttackMode, single, burst, thrust, slash, swing, throwing, punch, kick);

	// TODO: add cancel order
	DEFINE_ENUM(OrderTypeId, idle, look_at, move, track, attack, change_stance, interact, drop_item,
				equip_item, unequip_item, transfer_item, get_hit, die);

	// TODO: better name
	DEFINE_ENUM(GameModeId, single_player, death_match, trench_war, hunter);

	DEFINE_ENUM(HealthStatus, unhurt, barely_wounded, wounded, seriously_wounded, near_death, dead);

	const char *describe(HealthStatus);
	HealthStatus healthStatusFromHP(float);

	DEFINE_ENUM(MessageId, sound, actor_order, class_changed, update_client, remove_client, respawn,
				update_client_info);

	DEFINE_ENUM(SoundType, normal, explosion, shooting);

	inline constexpr bool isRanged(AttackMode t) {
		return isOneOf(t, AttackMode::single, AttackMode::burst, AttackMode::throwing);
	}
	inline constexpr bool isMelee(AttackMode t) { return !isRanged(t); }
	inline constexpr unsigned toFlags(AttackMode t) { return 1 << (uint)t; }

	using AttackModeFlags = EnumFlags<AttackMode>;
	Maybe<AttackMode> getFirst(AttackModeFlags);

	namespace Flags { enum Type : unsigned; };
	using FlagsType = Flags::Type;

	inline constexpr FlagsType entityIdToFlag(EntityId id) { return (FlagsType)(1u << (4 + (uint)id)); }
	inline constexpr FlagsType tileIdToFlag(TileId id) { return (FlagsType)(1u << (16 + (uint)id)); }

	static_assert(count<EntityId>() <= 12, "Flag limit reached");
	static_assert(count<TileId>() <= 8, "Flag limit reached");

	namespace Flags {
		enum Type :unsigned {
			//Generic flags: when testing, at least one flag must match
			static_entity	= 0x0001,
			dynamic_entity	= 0x0002, // can change position and/or bounding box

			container		= entityIdToFlag(EntityId::container),
			door			= entityIdToFlag(EntityId::door),
			actor			= entityIdToFlag(EntityId::actor),
			turret			= entityIdToFlag(EntityId::turret),
			item			= entityIdToFlag(EntityId::item),
			projectile		= entityIdToFlag(EntityId::projectile),
			impact			= entityIdToFlag(EntityId::impact),
			trigger			= entityIdToFlag(EntityId::trigger),

			entity			= 0xffff,

			wall_tile		= tileIdToFlag(TileId::wall),
			floor_tile		= tileIdToFlag(TileId::floor),
			object_tile		= tileIdToFlag(TileId::object),
			stairs_tile		= tileIdToFlag(TileId::stairs),
			roof_tile		= tileIdToFlag(TileId::roof),
			walkable_tile	= floor_tile | stairs_tile | roof_tile,

			tile			= 0xff0000,

			all				= 0xffffff,
	
			//Functional flags: when testing, all of the selected flags must match	
			visible			= 0x01000000,
			occluding		= 0x02000000,
			colliding		= 0x04000000,
		};

		inline constexpr Type operator|(Type a, Type b) { return (Type)((unsigned)a | (unsigned)b); }
		inline constexpr Type operator&(Type a, Type b) { return (Type)((unsigned)a & (unsigned)b); }
		inline constexpr Type operator~(Type a) { return (Type)(~(unsigned)a); }

		inline constexpr bool test(unsigned object_flags, Type test) {
			return (object_flags & test & 0xffffff) && ((object_flags & test & 0xff000000) == (test & 0xff000000));
		}
	
	}

	class SoundId {
	public:
		SoundId() :m_id(-1) { }
		SoundId(const char *sound_name, int offset = 0);
		operator int() const { return m_id; }
		bool isValid() const { return m_id != -1; }

	protected:
		int m_id;
	};

	// Object reference contains only pure indices, so they may be invalid
	// and refEntity / refTile methods in World may return null even if
	// isEntity / isTile returns true
	class ObjectRef {
	public:
		ObjectRef() :m_index(-1) { }
		explicit operator bool() const { return !empty(); }

		bool operator==(const ObjectRef &rhs) const
			{ return m_index == rhs.m_index && m_is_entity == rhs.m_is_entity; }

		bool empty() const { return m_index == -1; }
		bool isEntity() const { return !empty() && m_is_entity; }
		bool isTile() const { return !empty() && !m_is_entity; }
		int index() const { return m_index; }

	private:
		ObjectRef(int index, bool is_entity) :m_index(index), m_is_entity(is_entity? 1 : 0) { }

		int m_index: 31;
		int m_is_entity: 1;
		friend class World;
		friend class EntityRef;
		friend class WorldViewer;
	};

	class Intersection {
	public:
		Intersection(ObjectRef ref = ObjectRef(), float distance = fconstant::inf)
			:m_ref(ref), m_distance(distance) { }
		explicit operator bool() const { return !empty(); }
		bool operator==(const Intersection &rhs) const
			{ return empty()? rhs.empty() : m_ref == rhs.m_ref && m_distance == rhs.m_distance; }

		operator const ObjectRef() const { return m_ref; }
		bool empty() const { return m_ref.empty(); }
		bool isEntity() const { return m_ref.isEntity(); }
		bool isTile() const { return m_ref.isTile(); }

		float distance() const { return m_distance; }

	private:
		ObjectRef m_ref;
		float m_distance;
	};

}

#include "game/proto.h"
