/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include "base.h"
#include "game/item.h"
#include "game/weapon.h"
#include "game/armour.h"

namespace game {

	class Inventory {
	public:
		Inventory() = default;
		Inventory(const XMLNode&);
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
		const string printMenu(int select) const;

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
		ActorInventory(Weapon dummy_weapon);
		ActorInventory(Weapon dummy_weapon, const XMLNode&);

		void save(XMLNode) const;

		bool equip(int id, int count = 1);
		bool isEquipped(ItemType::Type);
		int unequip(ItemType::Type);
		bool isEmpty() const;

		const string printMenu(int select) const;
		float weight() const;

		const Weapon &dummyWeapon() const { return m_dummy_weapon; }
		static const Armour dummyArmour();
		static const Item   dummyAmmo();

		const Weapon &weapon() const { return m_weapon; }
		const Armour &armour() const { return m_armour; }
		const Entry  &ammo  () const { return m_ammo; }
		void useAmmo(int count);

		void save(Stream&) const;
		void load(Stream&);

	protected:
		Weapon m_weapon, m_dummy_weapon;
		Armour m_armour;
		Entry  m_ammo;
	};




}


#endif
