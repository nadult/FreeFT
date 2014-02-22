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
		DASSERT(!item.isDummy());

		if(!count)
			return -1;

		int entry_id = find(item);
		if(entry_id != -1) {
			m_entries[entry_id].count += count;
			return entry_id;
		}

		if(size() == max_entries)
			return -1;

		m_entries.emplace_back(Entry{item, count});
		return size() - 1;
	}

	void Inventory::remove(int entry_id, int count) {
		DASSERT(entry_id >= 0 && entry_id < size());
		DASSERT(count >= 0);

		Entry &entry = m_entries[entry_id];
		entry.count -= count;
		if(entry.count <= 0) {
			std::swap(m_entries[entry_id], m_entries.back());
			m_entries.pop_back();
		}
	}

	float Inventory::weight() const {
		double sum = 0.0;
		for(int n = 0; n < size(); n++)
			sum += m_entries[n].weight();
		return float(sum);
	}

	const string Inventory::printMenu(int select) const {
		char buf[1024], *ptr = buf, *end = buf + sizeof(buf);
		const char *sel = "(*) ", *desel = "( ) ";
		buf[0] = 0;

		for(int n = 0; n < size(); n++) {
			const Entry &entry = (*this)[n];
			ptr += snprintf(ptr, end - ptr, "%sitem: %s (type: %s)", select == n? sel : desel,
					entry.item.name().c_str(), ItemType::toString(entry.item.type()));
			ptr += snprintf(ptr, end - ptr, entry.count > 1? " [%d]\n" : "\n", entry.count);
		}

		return string(buf);
	}
		
	void Inventory::save(Stream &sr) const {
		sr.encodeInt(size());
		for(int n = 0; n < size(); n++) {
			sr.encodeInt(m_entries[n].count);
			sr << m_entries[n].item;
		}
	}

	void Inventory::load(Stream &sr) {
		int count = sr.decodeInt();
		ASSERT(count >= 0 && count <= max_entries);

		m_entries.clear();
		for(int n = 0; n < count; n++) {
			int icount = sr.decodeInt();
			ASSERT(icount > 0);

			Item item(sr);
			m_entries.emplace_back(Entry{item, icount});
		}
	}

	bool ActorInventory::equip(int id, int count) {
		DASSERT(isValidId(id) && count > 0);

		Item item = m_entries[id].item;
		ItemType::Type type = item.type();
		if((type != ItemType::weapon && type != ItemType::armour && type != ItemType::ammo) || !count)
			return false;

		unequip(type);

		count = min(type == ItemType::ammo? count : 1, m_entries[id].count);
		remove(id, count);

		if(type == ItemType::weapon)
			m_weapon = item;
		else if(type == ItemType::armour)
			m_armour = item;
		else if(type == ItemType::ammo) {
			m_ammo.item = item;
			m_ammo.count = count;
		}

		return true;
	}

	ActorInventory::ActorInventory()
		:m_weapon(dummyWeapon()), m_armour(dummyArmour()), m_ammo{dummyAmmo(), 0} { }
		
	const Weapon ActorInventory::dummyWeapon() {
		return Weapon(Item::find("_dummy_weapon", ItemType::weapon));
	}

	const Armour ActorInventory::dummyArmour() {
		return Armour(Item::find("_dummy_armour", ItemType::armour));
	}

	const Item   ActorInventory::dummyAmmo() {
		return Item(Item::find("_dummy_ammo", ItemType::ammo), ItemType::ammo);
	}

	int ActorInventory::unequip(ItemType::Type item_type) {
		int ret = -1;

		if(item_type == ItemType::weapon) {
			if(!m_weapon.isDummy()) {
				ret = add(m_weapon, 1);
				m_weapon = dummyWeapon();
			}
		}
		else if(item_type == ItemType::armour) {
			if(!m_armour.isDummy()) {
				ret = add(m_armour, 1);
				m_armour = dummyArmour();
			}
		}
		else if(item_type == ItemType::ammo) {
			if(!m_ammo.item.isDummy()) {
				ret = add(m_ammo.item, m_ammo.count);
				m_ammo.item = dummyWeapon();
				m_ammo.count = 0;
			}
		}

		return ret;
	}
		
	void ActorInventory::useAmmo(int count) {
		DASSERT(count >= 0);
		m_ammo.count = max(0, m_ammo.count - count);
	}

	float ActorInventory::weight() const {
		float weight = Inventory::weight();

		weight += m_weapon.weight();
		weight += m_armour.weight();
		weight += m_ammo.weight();

		return weight;
	}

	const string ActorInventory::printMenu(int select) const {
		char buf[1024], *ptr = buf, *end = buf + sizeof(buf);
		const char *sel = "(*) ", *desel = "( ) ";

		ptr += snprintf(ptr, end - ptr, "%sweapon: %s\n", select == -3? sel : desel,
				m_weapon.isDummy()? "none" : m_weapon.name().c_str());
		ptr += snprintf(ptr, end - ptr, "%sarmour: %s\n", select == -2? sel : desel,
				m_armour.isDummy()? "none" : m_armour.name().c_str());
		ptr += snprintf(ptr, end - ptr, "%sammo: %s [%d]\n", select == -1? sel : desel,
				m_ammo.item.isDummy()? "none" : m_ammo.item.name().c_str(), m_ammo.count);

		return string(buf) + Inventory::printMenu(select);
	}
	
	void ActorInventory::save(Stream &sr) const {
		Inventory::save(sr);
		sr << u8(	(!m_weapon.isDummy()? 1 : 0) |
					(!m_armour.isDummy()? 2 : 0) |
					(!m_ammo.item.isDummy()? 4 : 0) );

		if(!m_weapon.isDummy())
			sr << m_weapon;
		if(!m_armour.isDummy())
			sr << m_armour;
		if(!m_ammo.item.isDummy()) {
			sr << m_ammo.item;
			sr.encodeInt(m_ammo.count);
		}
	}

	void ActorInventory::load(Stream &sr) {
		Inventory::load(sr);
		u8 flags;
		sr >> flags;

		m_weapon = flags & 1? Weapon(Item(sr)) : dummyWeapon();
		m_armour = flags & 2? Armour(Item(sr)) : dummyArmour();
		m_ammo.item = flags & 4? Item(sr) : dummyAmmo();
		m_ammo.count = flags & 4? sr.decodeInt() : 0;
	}
}
