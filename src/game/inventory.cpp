/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/inventory.h"
#include <cstdio>

namespace game {

	int Inventory::find(const Item &item) const {
		for(int n = 0; n < size(); n++)
			if(m_entries[n].item == item)
				return n;
		return -1;
	}

	int Inventory::add(const Item &item, int count) {
		DASSERT(count >= 0);
		if(!count)
			return -1;

		int entry_id = find(item);
		if(entry_id != -1) {
			m_entries[entry_id].count += count;
			return entry_id;
		}

		Entry new_entry;
		new_entry.item = item;
		new_entry.count = count;
		m_entries.push_back(new_entry);
		return size() - 1;
	}

	void Inventory::remove(int entry_id, int count) {
		DASSERT(entry_id >= 0 && entry_id < size());
		DASSERT(count >= 0);

		Entry &entry = m_entries[entry_id];
		entry.count -= count;
		if(entry.count <= 0) {
			m_entries[entry_id] = m_entries.back();
			m_entries.pop_back();
		}
	}

	float Inventory::weight() const {
		double sum = 0.0;
		for(int n = 0; n < size(); n++)
			sum += double(m_entries[n].item.weight()) * double(m_entries[n].count);
		return float(sum);
	}

	const string Inventory::printMenu(int select) const {
		char buf[1024], *ptr = buf, *end = buf + sizeof(buf);
		const char *sel = "(*) ", *desel = "( ) ";
		buf[0] = 0;

		for(int n = 0; n < size(); n++) {
			const Entry &entry = (*this)[n];
			ptr += snprintf(ptr, end - ptr, "%sitem: %s (type: %s)", select == n? sel : desel,
					entry.item.name(), ItemTypeId::toString(entry.item.typeId()));
			ptr += snprintf(ptr, end - ptr, entry.count > 1? " [%d]\n" : "\n", entry.count);
		}

		return string(buf);
	}


	InventorySlotId::Type ActorInventory::equip(int id) {
		DASSERT(id >= 0 && id < size());
		Item item = m_entries[id].item;
		InventorySlotId::Type slot_id =
			item.typeId() == ItemTypeId::weapon? InventorySlotId::weapon :
			item.typeId() == ItemTypeId::armour? InventorySlotId::armour :
			item.typeId() == ItemTypeId::ammo? InventorySlotId::ammo : InventorySlotId::invalid;

		if(slot_id == InventorySlotId::invalid)
			return InventorySlotId::invalid;

		if(m_slots[slot_id].count)
			unequip(slot_id);
		remove(id, 1);

		m_slots[slot_id].item = item;
		m_slots[slot_id].count = 1;
		return slot_id;
	}

	int ActorInventory::unequip(InventorySlotId::Type slot_id) {
		DASSERT(slot_id >= 0 && slot_id < InventorySlotId::count);
		if(m_slots[slot_id].count == 0)
			return -1;
		int id = add(m_slots[slot_id].item, m_slots[slot_id].count);
		m_slots[slot_id].item = Item();
		m_slots[slot_id].count = 0;
		return id;
	}

	float ActorInventory::weight() const {
		float weight = Inventory::weight();
		for(int n = 0; n < InventorySlotId::count; n++)
			if(m_slots[n].count)
				weight += m_slots[n].item.weight() * float(m_slots[n].count);
		return weight;
	}

	const string ActorInventory::printMenu(int select) const {
		char buf[1024], *ptr = buf, *end = buf + sizeof(buf);
		const char *sel = "(*) ", *desel = "( ) ";
		ptr += snprintf(ptr, end - ptr, "%sarmour: %s\n", select == -2? sel : desel,
				armour().isValid()? armour().name() : "none");
		ptr += snprintf(ptr, end - ptr, "%sweapon: %s\n", select == -1? sel : desel,
				weapon().isValid()? weapon().name() : "none");
		return string(buf) + Inventory::printMenu(select);
	}

}
