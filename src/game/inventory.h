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
		enum { max_entries = 1024 };

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

		bool equip(int id, int count = 1);
		bool isEquipped(ItemType::Type);
		int unequip(ItemType::Type);


		const string printMenu(int select) const;
		float weight() const;

		static const Weapon dummyWeapon();
		static const Armour dummyArmour();
		static const Item   dummyAmmo();

		const Weapon &weapon() const { return m_weapon; }
		const Armour &armour() const { return m_armour; }
		const Entry  &ammo  () const { return m_ammo; }
		void useAmmo(int count);

		void save(Stream&) const;
		void load(Stream&);

	protected:
		Weapon m_weapon;
		Armour m_armour;
		Entry  m_ammo;
	};




}


#endif
