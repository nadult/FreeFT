#ifndef GaME_ITEM_H
#define GAME_ITEM_H

#include "game/entity.h"
#include "game/projectile.h"
#include "gfx/device.h"

namespace game {

	namespace ItemType {
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

	struct ItemDesc {
		virtual ~ItemDesc() { }
		ItemType::Type type() const { return ItemType::other; }

		static void loadItems();
		static const ItemDesc *find(const char *name);

		float weight;
		string description;
		string name;
		string sprite_name;
	};

	typedef std::unique_ptr<ItemDesc> PItemDesc;

	struct WeaponDesc: public ItemDesc {
		ItemType::Type type() const { return ItemType::weapon; }

		ProjectileType::Type projectile_type;
		float projectile_speed;
		float damage;
	};

	struct AmmoDesc: public ItemDesc {
		ItemType::Type type() const { return ItemType::ammo; }

		float damage_modifier;
	};

	struct ArmourDesc: public ItemDesc {
		ItemType::Type type() const { return ItemType::armour; }

		float damage_resistance;
	};

	class Inventory {
	public:
		struct Entry {
			const ItemDesc *item;
			int count;
		};

		int addItem(const ItemDesc *item, int count);
		int findItem(const ItemDesc *item);
		void remove(int entry_id, int count);
		float weight() const;

		int size() const { return (int)m_entries.size(); }
		const Entry &operator[](int idx) const { return m_entries[idx]; }

	protected:
		vector<Entry> m_entries;
	};


	class Item: public Entity {
	public:
		Item(const ItemDesc &desc, const float3 &pos);
		ColliderFlags colliderType() const { return collider_dynamic; }
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
