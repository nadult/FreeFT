/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/character.h"
#include "game/inventory.h"
#include "gfx/device.h"
#include "sys/platform.h"
#include "sys/xml.h"

namespace game {

	static const char *s_icon_folder = "char/";
	static const char *s_empty_name = "no_char";

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
				return gfx::DTexture::gui_mgr[string(s_icon_folder) + m_icon_name];
			}
			catch(...) { } //TODO: log error
		}

		return emptyIcon();
	}
		
	gfx::PTexture Character::emptyIcon() {
		return gfx::DTexture::gui_mgr[string(s_icon_folder) + s_empty_name];
	}
		
	const vector<pair<ProtoIndex, string>> Character::findIcons() {
		XMLDocument doc;
		Loader("data/char_icons.xml") >> doc;

		vector<pair<ProtoIndex, string>> out;

		XMLNode node = doc.child("char");
		while(node) {
			const char *proto_name = node.attrib("proto");
			const char *icon_name = node.attrib("icon");
			out.push_back(make_pair(findProto(proto_name, ProtoId::actor), string(icon_name)));
			node = node.sibling("char");
		}

		return out;
	}
		
	bool Character::operator==(const Character &rhs) const {
		return m_name == rhs.m_name && m_icon_name == rhs.m_icon_name && m_proto_idx == rhs.m_proto_idx;
	}
		
	struct StartingEquipment {
		int tier;
		const char *class_name;

		const char *armour;
		const char *weapon;
		const char *ammo;
		int ammo_count;
	};

	static StartingEquipment s_equipments[] = {
		{ 0, "Scout",		"leather_armour",		"uzi",				"9mm_ball",		200 },
		{ 0, "Infantry",	nullptr,				"ak47",				"762mm",		150 },
		{ 0, "Sniper",		nullptr,				"laser_rifle",		"fusion_cell",	100 }
	};

	int CharacterClass::count() {
		return COUNTOF(s_equipments);
	}

	const string CharacterClass::name() const {
		return s_equipments[m_id].class_name;
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
		
	bool CharacterClass::operator==(const CharacterClass &rhs) const {
		return m_tier == rhs.m_tier && m_id == rhs.m_id;
	}

	PlayableCharacter::PlayableCharacter(const Character &character)
		:m_character(character), m_class(0) { }

	PlayableCharacter::~PlayableCharacter() { }
		
	PlayableCharacter::PlayableCharacter(Stream &sr) :m_character(sr), m_class(0) {
		sr >> m_entity_ref;
		int class_id = sr.decodeInt();
		ASSERT(CharacterClass::isValidId(class_id));
		m_class = CharacterClass(class_id);
	}

	void PlayableCharacter::save(Stream &sr) const {
		sr << m_character << m_entity_ref;
		sr.encodeInt(m_class.id());
	}

	void PlayableCharacter::load(Stream &sr) {
		*this = PlayableCharacter(sr);
	}
		
	bool PlayableCharacter::operator==(const PlayableCharacter &rhs) const {
		return m_character == rhs.m_character && m_class == rhs.m_class && m_entity_ref == rhs.m_entity_ref;
	}
	

}
