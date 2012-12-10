#ifndef GaME_ITEM_H
#define GAME_ITEM_H

#include "game/entity.h"
#include "gfx/device.h"

namespace game {

	struct ItemDesc {
		virtual ~ItemDesc() { }
		ItemTypeId::Type type() const { return ItemTypeId::other; }

		static void loadItems();
		static const ItemDesc *find(const char *name);

		float weight;
		string description;
		string name;
		string sprite_name;
	};

	typedef std::unique_ptr<ItemDesc> PItemDesc;

	struct WeaponDesc: public ItemDesc {
		ItemTypeId::Type type() const { return ItemTypeId::weapon; }

		ProjectileTypeId::Type projectile_type;
		float projectile_speed;
		float damage;
	};

	struct AmmoDesc: public ItemDesc {
		ItemTypeId::Type type() const { return ItemTypeId::ammo; }

		float damage_modifier;
	};

	struct ArmourDesc: public ItemDesc {
		ItemTypeId::Type type() const { return ItemTypeId::armour; }

		float damage_resistance;
	};

	class Item: public Entity {
	public:
		Item(const ItemDesc &desc, const float3 &pos);
		ColliderFlags colliderType() const { return collider_none; }
		virtual EntityFlags entityType() const { return entity_item; }

		gfx::PTexture guiImage(bool small) const;
		const ItemDesc &desc() const { return m_desc; }

	private:
		int m_seq_ids[3];
		const ItemDesc &m_desc;
	};

	typedef Ptr<Item> PItem;

}



#endif
