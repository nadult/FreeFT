#ifndef GAME_ENUMS_H
#define GAME_ENUMS_H

namespace game {

	namespace WeaponClassId {
		enum Type {
			unarmed,
			club,
			heavy,
			knife,
			minigun,
			pistol,
			rifle,
			rocket,
			smg,
			spear,
			
			count,
		};

		const char *toString(Type);
		Type fromString(const char*);
	};

	namespace ItemTypeId {
		enum Type {
			weapon,
			ammo,
			armour,
			other,

			count,
		};

		const char *toString(Type);
		Type fromString(const char*);
	};

	namespace ProjectileTypeId {
		enum Type {
			bullet,
			plasma,
			laser,
			rocket,

			count,
		};

		const char *toString(Type);
		Type fromString(const char*);
	};

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
