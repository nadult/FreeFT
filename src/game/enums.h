#ifndef GAME_ENUMS_H
#define GAME_ENUMS_H

namespace game {

	#define DECLARE_ENUM(type, ...) \
		namespace type { enum Type { __VA_ARGS__, count }; \
			const char *toString(Type); \
			Type fromString(const char*); \
			inline bool isValid(Type val) { return val >= 0 && val < count; } \
		}

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
	);

	DECLARE_ENUM(ArmourClassId,
		none,
		leather,
		metal,
		environmental,
		power
	);

	DECLARE_ENUM(ItemTypeId,
		invalid = -1,
		weapon = 0,
		armour,
		ammo,
		other
	);

	DECLARE_ENUM(InventorySlotId,
		invalid = -1,
		weapon = 0,
		armour,
		ammo
	);

	DECLARE_ENUM(ActorTypeId,
		male,
		female,
		ghoul
	);

	DECLARE_ENUM(ProjectileTypeId,
		bullet,
		plasma,
		laser,
		rocket
	);

#undef DECLARE_ENUM

	enum EntityFlags {
		entity_container	= 1,
		entity_door			= 2,
		entity_actor		= 4,
		entity_item			= 8,
		entity_projectile	= 16,
		entity_impact		= 32,
	};

	enum ColliderFlags {
		collider_none		= 0,
		collider_tiles		= 1,

		collider_static		= 2, // updates NavigationMap when its being fully recomputed
		collider_dynamic_nv	= 4, // updates NavigationMap every frame
		collider_dynamic	= 8, // does not update NavigationMap

		collider_entities = collider_static | collider_dynamic | collider_dynamic_nv,

		collider_all		= 0xffffffff,
	};
	
	inline ColliderFlags operator|(ColliderFlags a, ColliderFlags b) { return (ColliderFlags)((int)a | (int)b); }

}

#endif
