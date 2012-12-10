#ifndef GAME_ITEM_H
#define GAME_ITEM_H

#include "game/entity.h"
#include "gfx/device.h"

namespace game {

	union ItemParameter {
		int i;
		float f;
	};

	struct ItemDesc {
		enum { param_count = 4 };

		virtual ~ItemDesc() { }
		virtual ItemTypeId::Type type() const { return ItemTypeId::other; }

		static void loadItems();
		static const ItemDesc *find(const char *name);

		virtual void initialize(ItemParameter*) const;

		float weight;
		string description;
		string name;
		string sprite_name;
	};

	typedef std::unique_ptr<ItemDesc> PItemDesc;

	struct WeaponDesc: public ItemDesc {
		ItemTypeId::Type type() const { return ItemTypeId::weapon; }

		ProjectileTypeId::Type projectile_type;
		WeaponClassId::Type class_id;
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

	struct Item
	{
	public:
		Item(const ItemDesc *desc = nullptr);

		bool isValid() const { return m_desc != nullptr; }
		float weight() const;

		bool operator==(const Item&) const;
		bool operator!=(const Item &rhs) const { return !(*this == rhs); }

		ItemTypeId::Type typeId() const;
		const char *spriteName() const;
		const ItemDesc *desc() const { return m_desc; }

	protected:
		enum { param_count = ItemDesc::param_count };

		const ItemDesc *m_desc;

		// These params should be used as arguments to
		// specific methods in ItemDesc
		ItemParameter params[ItemDesc::param_count];
	};

	class ItemEntity: public Entity {
	public:
		ItemEntity(const Item &item, const float3 &pos);
		ColliderFlags colliderType() const { return collider_none; }
		virtual EntityFlags entityType() const { return entity_item; }

		gfx::PTexture guiImage(bool small) const;
		const Item &item() const { return m_item; }
		Item &item() { return m_item; }

	private:
		int m_seq_ids[3];
		Item m_item;
	};

	typedef Ptr<ItemEntity> PItem;

}



#endif
