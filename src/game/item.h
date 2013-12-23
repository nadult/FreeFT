/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

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
		static const ItemDesc *get(int id);
		static int count();

		virtual void initialize(ItemParameter*) const;

		float weight;
		string id;
		string description;
		string name;
		string sprite_name;
	};

	typedef std::unique_ptr<ItemDesc> PItemDesc;

	struct WeaponDesc: public ItemDesc {
		ItemTypeId::Type type() const { return ItemTypeId::weapon; }

		ProjectileTypeId::Type projectile_type_id;
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

		ArmourClassId::Type class_id;
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
		const char *name() const;

		const ItemDesc *desc() const { return m_desc; }

	protected:
		enum { param_count = ItemDesc::param_count };

		const ItemDesc *m_desc;

		// These params should be used as arguments to
		// specific methods in ItemDesc
		ItemParameter params[ItemDesc::param_count];
	};

	struct Weapon: public Item
	{
	public:
		Weapon(const Item&);

		ProjectileTypeId::Type projectileTypeId() const;
		WeaponClassId::Type classId() const;
		float projectileSpeed() const;

		const WeaponDesc *desc() const { return static_cast<const WeaponDesc*>(m_desc); }
	};

	struct Armour: public Item
	{
	public:
		Armour(const Item&);

		ArmourClassId::Type classId() const;

		const ArmourDesc *desc() const { return static_cast<const ArmourDesc*>(m_desc); }
	};

	static_assert(sizeof(Weapon) == sizeof(Item), "Weapon should be a simple interface built on top of Item");
	static_assert(sizeof(Armour) == sizeof(Item), "Armour should be a simple interface built on top of Item");

	class ItemEntity: public Entity {
	public:
		ItemEntity(Stream&);
		ItemEntity(const Item &item, const float3 &pos) { initialize(item, pos); }

		ColliderFlags colliderType() const { return collider_item; }
		virtual EntityId::Type entityType() const { return EntityId::item; }
		virtual Entity *clone() const;

		gfx::PTexture guiImage(bool small, FRect &tex_image) const;
		const Item &item() const { return m_item; }
		Item &item() { return m_item; }
		
	private:
		void initialize(const Item &item, const float3 &pos);

		virtual void saveContentsToXML(XMLNode&) const;
		virtual void saveToBinary(Stream&);

		int m_seq_ids[3];
		Item m_item;
	};

	typedef Ptr<ItemEntity> PItem;

}



#endif
