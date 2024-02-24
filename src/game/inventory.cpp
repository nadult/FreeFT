// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/inventory.h"
#include <cstdio>

namespace game {

Inventory::Inventory(CXmlNode node) {
	if(!node)
		return;

	auto child = node.child("item");

	while(child) {
		Entry new_entry;
		new_entry.item = Item(ProtoIndex(child));
		if(auto count_attrib = child.tryAttrib("count"))
			new_entry.count = fromString<int>(count_attrib);
		else
			new_entry.count = 1;
		m_entries.push_back(new_entry);
		child = child.sibling("item");
	}
}

void Inventory::save(XmlNode node) const {
	for(int n = 0; n < (int)m_entries.size(); n++) {
		auto item_node = node.addChild("item");
		m_entries[n].item.index().save(item_node);
		if(m_entries[n].count != 1)
			item_node.addAttrib("count", m_entries[n].count);
	}
}

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

	Entry &entry = m_entries[entry_id];
	DASSERT(count >= 0 && count <= entry.count);

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

void Inventory::save(MemoryStream &sr) const {
	encodeInt(sr, size());
	for(int n = 0; n < size(); n++) {
		encodeInt(sr, m_entries[n].count);
		m_entries[n].item.save(sr);
	}
}

void Inventory::load(MemoryStream &sr) {
	int count = decodeInt(sr);
	ASSERT(count >= 0 && count <= max_entries);

	m_entries.clear();
	for(int n = 0; n < count; n++) {
		int icount = decodeInt(sr);
		ASSERT(icount > 0);

		Item item(sr);
		m_entries.emplace_back(Entry{item, icount});
	}
}

ActorInventory::ActorInventory()
	: m_weapon(Item::dummyWeapon()), m_dummy_weapon(Item::dummyWeapon()),
	  m_armour(Item::dummyArmour()), m_ammo{Item::dummyAmmo(), 0} {}

ActorInventory::ActorInventory(CXmlNode node)
	: Inventory(node), m_weapon(Item::dummyWeapon()), m_dummy_weapon(Item::dummyWeapon()),
	  m_armour(Item::dummyArmour()), m_ammo{Item::dummyAmmo(), 0} {
	if(!node)
		return;

	auto weapon_node = node.child("weapon");
	auto armour_node = node.child("armour");
	auto ammo_node = node.child("ammo");

	if(weapon_node)
		m_weapon = Item(findProto(weapon_node.attrib("proto_id"), ProtoId::weapon));
	if(armour_node)
		m_armour = Item(findProto(armour_node.attrib("proto_id"), ProtoId::armour));
	if(ammo_node) {
		Item ammo(findProto(ammo_node.attrib("proto_id"), ProtoId::ammo));
		int count = ammo_node.attrib<int>("count");
		int id = add(ammo, count);
		equip(id, count);
	}
}

void ActorInventory::save(XmlNode node) const {
	Inventory::save(node);
	if(!m_weapon.isDummy())
		m_weapon.index().save(node.addChild("weapon"));
	if(!m_armour.isDummy())
		m_armour.index().save(node.addChild("armour"));
	if(!m_ammo.item.isDummy()) {
		auto item_node = node.addChild("ammo");
		m_ammo.item.index().save(item_node);
		item_node.addAttrib("count", m_ammo.count);
	}
}

bool ActorInventory::equip(int id, int count) {
	DASSERT(isValidId(id) && count > 0);

	Item item = m_entries[id].item;
	ItemType type = item.type();
	if((type != ItemType::weapon && type != ItemType::armour && type != ItemType::ammo) || !count)
		return false;
	if(type == ItemType::ammo && m_weapon.proto().ammo_class_id != Ammo(item).classId())
		return false;

	Entry old_ammo = m_ammo;
	unequip(type);

	if(type == ItemType::ammo) {
		if(old_ammo.item == item)
			count += old_ammo.count;
		count = min(count, m_weapon.maxAmmo());
	} else
		count = min(count, 1);
	count = min(count, m_entries[id].count);

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

bool ActorInventory::isEquipped(ItemType item_type) {
	if(item_type == ItemType::weapon)
		return !m_weapon.isDummy();
	else if(item_type == ItemType::armour)
		return !m_armour.isDummy();
	else if(item_type == ItemType::ammo)
		return !m_ammo.item.isDummy();

	return false;
}

int ActorInventory::unequip(ItemType item_type) {
	int ret = -1;

	if(item_type == ItemType::weapon) {
		if(!m_weapon.isDummy()) {
			ret = add(m_weapon, 1);
			m_weapon = m_dummy_weapon;
		}

		unequip(ItemType::ammo);
	} else if(item_type == ItemType::armour) {
		if(!m_armour.isDummy()) {
			ret = add(m_armour, 1);
			m_armour = Item::dummyArmour();
		}
	} else if(item_type == ItemType::ammo) {
		if(!m_ammo.item.isDummy()) {
			ret = add(m_ammo.item, m_ammo.count);
			m_ammo.item = Item::dummyAmmo();
			m_ammo.count = 0;
		}
	}

	return ret;
}

int ActorInventory::findAmmo(const Weapon &weapon) const {
	for(int n = 0; n < size(); n++)
		if(weapon.canUseAmmo((*this)[n].item))
			return n;
	return -1;
}

bool ActorInventory::useAmmo(int count) {
	DASSERT(count >= 0);
	if(m_ammo.count < count)
		return false;
	m_ammo.count -= count;
	return true;
}

float ActorInventory::weight() const {
	float weight = Inventory::weight();

	weight += m_weapon.weight();
	weight += m_armour.weight();
	weight += m_ammo.weight();

	return weight;
}

bool ActorInventory::empty() const {
	return Inventory::empty() && m_weapon.isDummy() && m_armour.isDummy() &&
		   (m_ammo.item.isDummy() || m_ammo.count == 0);
}

void ActorInventory::save(MemoryStream &sr) const {
	Inventory::save(sr);
	sr << u8((!m_weapon.isDummy() ? 1 : 0) | (!m_armour.isDummy() ? 2 : 0) |
			 (!m_ammo.item.isDummy() ? 4 : 0));

	if(!m_weapon.isDummy())
		m_weapon.save(sr);
	if(!m_armour.isDummy())
		m_armour.save(sr);
	if(!m_ammo.item.isDummy()) {
		m_ammo.item.save(sr);
		encodeInt(sr, m_ammo.count);
	}
}

void ActorInventory::load(MemoryStream &sr) {
	Inventory::load(sr);
	u8 flags;
	sr >> flags;

	m_weapon = flags & 1 ? Weapon(Item(sr)) : m_dummy_weapon;
	m_armour = flags & 2 ? Armour(Item(sr)) : Item::dummyArmour();
	m_ammo.item = flags & 4 ? Item(sr) : Item::dummyAmmo();
	m_ammo.count = flags & 4 ? decodeInt(sr) : 0;
}

void ActorInventory::setDummyWeapon(Weapon dummy) {
	if(m_weapon.isDummy())
		m_weapon = dummy;
	m_dummy_weapon = dummy;
}

}
