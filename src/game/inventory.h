/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include "base.h"
#include "game/item.h"
#include "game/weapon.h"
#include "game/armour.h"
#include "game/ammo.h"

namespace game {

	class Inventory {
	public:
		Inventory() = default;
		Inventory(const XMLNode&);

		//TODO: remove this limit
		enum { max_entries = 1024 };
		
		void save(XMLNode) const;

		struct Entry {
			float weight() const { return item.weight() * float(count); }

			Item item;
			int count;
		};

		int add(const Item &item, int count);
		int find(const Item &item) const;
		void remove(int entry_id, int count);
		float weight() const;

		bool isValidId(int id) const { return id >= 0 && id < size(); }
		bool isEmpty() const { return m_entries.empty(); }

		int size() const { return (int)m_entries.size(); }
		const Entry &operator[](int idx) const { return m_entries[idx]; }

		void save(Stream&) const;
		void load(Stream&);

	protected:
		vector<Entry> m_entries;
	};

	class ActorInventory: public Inventory {
	public:
		ActorInventory();
		ActorInventory(const XMLNode&);

		void save(XMLNode) const;

		bool equip(int id, int count = 1);
		bool isEquipped(ItemType::Type);
		int unequip(ItemType::Type);
		bool isEmpty() const;
		float weight() const;

		int findAmmo(const Weapon &weapon) const;

		const Weapon &dummyWeapon() const { return m_dummy_weapon; }
		void setDummyWeapon(Weapon);

		const Weapon &weapon() const { return m_weapon; }
		const Armour &armour() const { return m_armour; }
		const Entry  &ammo  () const { return m_ammo; }
		bool useAmmo(int count);

		void save(Stream&) const;
		void load(Stream&);
		
		static int predefinedCount();
		// Returns a pair: inventory and tier id
		static pair<ActorInventory, int> getPredefined(int id, bool equip_items);

	protected:
		Weapon m_weapon, m_dummy_weapon;
		Armour m_armour;
		Entry  m_ammo;
	};

}

#endif
