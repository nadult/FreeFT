/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/character.h"
#include "game/inventory.h"
#include "gfx/device.h"

namespace game {

	Character::Character(const string &name, const string &icon_name, const string &proto_name)
   		:m_name(name), m_icon_name(icon_name), m_proto_idx(findProto(proto_name, ProtoId::actor)) {
		validate();
	}
		
	Character::Character(Stream &sr) { load(sr); }
		
	void Character::validate() {
		ASSERT(m_name.size() <= max_name_size);
		ASSERT(m_icon_name.size() < max_icon_name_size);
		ASSERT(m_proto_idx.isValid());
	}

	void Character::save(Stream &sr) const {
		sr << m_name << m_icon_name << m_proto_idx;
	}

	void Character::load(Stream &sr) {
		sr >> m_name >> m_icon_name;
		m_proto_idx = ProtoIndex(sr);
		validate();
	}
		
	gfx::PTexture Character::icon() const {
		if(!m_icon_name.empty()) {
			try {
				return gfx::DTexture::gui_mgr[string("char/") + m_icon_name];
			}
			catch(...) { } //TODO: log error
		}

		return defaultIcon();
	}
		
	gfx::PTexture Character::defaultIcon() {
		return gfx::DTexture::gui_mgr["char/no_char"];
	}
		
	struct StartingEquipment {
		int tier;
		const char *armour;
		const char *weapon;
		const char *ammo;
		int ammo_count;
	};

	static StartingEquipment s_equipments[] = {
		{ 0, "leather_armour",		"uzi",				"9mm_ball",		200 },
		{ 0, nullptr,				"ak47",				"762mm",		150 },
		{ 0, nullptr,				"plasma_rifle",		"fusion_cell",	100 }
	};

	int CharacterClass::count() {
		return COUNTOF(s_equipments);
	}
		
	bool CharacterClass::isValidId(int id) {
		return id >= 0 && id < count();
	}

	CharacterClass::CharacterClass(int class_id) :m_id(class_id) {
		DASSERT(isValidId(class_id));
		m_tier = s_equipments[m_id].tier;
	}

	const ActorInventory CharacterClass::inventory(bool equip_items) const {
		const StartingEquipment &def = s_equipments[m_id];

		ActorInventory out;
		if(def.armour) {
			int id = out.add(findProto(def.armour, ProtoId::item_armour), 1);
			if(equip_items)
				out.equip(id);
		}
		if(def.weapon) {
			int id = out.add(findProto(def.weapon, ProtoId::item_weapon), 1);
			if(equip_items)
				out.equip(id);
			
			if(def.ammo && def.ammo_count > 0) {
				int ammo_id = out.add(findProto(def.ammo, ProtoId::item_ammo), def.ammo_count);
				if(equip_items)
					out.equip(ammo_id, min(def.ammo_count, out.weapon().maxAmmo()));
			}
		}

		return out;
	}

}
