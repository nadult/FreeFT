#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include "base.h"
#include "game/item.h"

namespace game {

	class Inventory {
	public:
		struct Entry {
			Entry() :count(0) { }

			Item item;
			int count;
		};

		int add(const Item &item, int count);
		int find(const Item &item);
		void remove(int entry_id, int count);
		float weight() const;

		int size() const { return (int)m_entries.size(); }
		const Entry &operator[](int idx) const { return m_entries[idx]; }

	protected:
		vector<Entry> m_entries;
	};

	class ActorInventory: public Inventory {
	public:
		InventorySlotId::Type equip(int id);
		int unequip(InventorySlotId::Type);

		const string printMenu(int select) const;
		float weight() const;

		const Weapon &weapon() const
			{ return reinterpret_cast<const Weapon&>(m_slots[InventorySlotId::weapon].item); }
		const Armour &armour() const
			{ return reinterpret_cast<const Armour&>(m_slots[InventorySlotId::armour].item); }
		const Item &ammo() const
			{ return m_slots[InventorySlotId::ammo].item; }

		const Entry &slot(InventorySlotId::Type id) const { return m_slots[id]; }

	protected:
		Entry m_slots[InventorySlotId::count];
	};




}


#endif
