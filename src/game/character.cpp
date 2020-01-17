// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/character.h"

#include "game/inventory.h"
#include <fwk/gfx/gl_texture.h>
#include <fwk/sys/xml.h>

namespace game {

	static const char *s_icon_folder = "char/";
	static const char *s_empty_name = "no_char";

	Character::Character(const string &name, const string &icon_name, const string &proto_name)
   		:m_name(name), m_icon_name(icon_name), m_proto_idx(findProto(proto_name, ProtoId::actor)) {
		validate();
	}
		
	Character::Character(MemoryStream &sr) { load(sr); }
		
	void Character::validate() {
		ASSERT(m_name.size() <= max_name_size);
		ASSERT(m_icon_name.size() < max_icon_name_size);
		ASSERT(m_proto_idx.isValid());
	}

	void Character::save(MemoryStream &sr) const {
		sr << m_name << m_icon_name << m_proto_idx;
	}

	void Character::load(MemoryStream &sr) {
		sr >> m_name >> m_icon_name;
		m_proto_idx = ProtoIndex(sr);
		validate();
	}
		
	PTexture Character::icon() const {
		if(!m_icon_name.empty()) {
			{
				return res::getGuiTexture(string(s_icon_folder) + m_icon_name);
			} //TODO: handle missing icon
		}

		return emptyIcon();
	}
		
	PTexture Character::emptyIcon() {
		return res::getGuiTexture(string(s_icon_folder) + s_empty_name);
	}
		
	const vector<pair<ProtoIndex, string>> Character::findIcons() {
		auto doc = move(XmlDocument::load("data/char_icons.xml").get()); //TODO

		vector<pair<ProtoIndex, string>> out;

		auto node = doc.child("char_icon");
		while(node) {
			auto proto_name = node.attrib("proto");
			auto icon_name = node.attrib("icon");
			out.push_back(make_pair(findProto(proto_name, ProtoId::actor), string(icon_name)));
			node = node.sibling("char_icon");
		}

		return out;
	}
		
	const vector<string> Character::findIcons(const string &proto_name) {
		const auto &pairs = findIcons();
		ProtoIndex index = findProto(proto_name, ProtoId::actor);

		vector<string> out;
		for(const auto &pair : pairs)
			if(pair.first == index)
				out.emplace_back(pair.second);
		return out;
	}
		
	bool Character::operator==(const Character &rhs) const {
		return m_name == rhs.m_name && m_icon_name == rhs.m_icon_name && m_proto_idx == rhs.m_proto_idx;
	}
		

	CharacterClass::CharacterClass(XmlNode node, int id) :m_inventory(node.child("inventory")) {
		m_name = node.attrib("name");
		m_tier = node.attrib<int>("tier");
		m_id = id;
		m_proto_names = node.attrib<vector<string>>("actors");
	}
		
	const ActorInventory CharacterClass::inventory(bool equip) const {
		ActorInventory out = m_inventory;
		if(!equip) {
			out.unequip(ItemType::weapon);
			out.unequip(ItemType::armour);
		}
		return out;
	}

	bool CharacterClass::isValidForActor(const string &proto_name) const {
		return m_proto_names.empty() || isOneOf(proto_name, m_proto_names);
	}

	static vector<CharacterClass> s_classes;

	void CharacterClass::loadAll() {
		if(!s_classes.empty())
			return;
		
		auto doc = move(XmlDocument::load("data/char_classes.xml").get()); // TODO
		
		auto class_node = doc.child("char_class");
		int counter = 0;

		while(class_node) {
			CharacterClass new_class(class_node, counter++);
			s_classes.emplace_back(new_class);
			class_node = class_node.sibling("char_class");
		}
	}
		
	int CharacterClass::count() {
		return (int)s_classes.size();
	}

	const CharacterClass &CharacterClass::get(int id) {
		DASSERT(id >= 0 && id < (int)s_classes.size());
		return s_classes[id];
	}


	PlayableCharacter::PlayableCharacter(const Character &character, int class_id)
		:m_character(character), m_class_id( (DASSERT(CharacterClass::isValidId(class_id)), class_id) ) { }

	PlayableCharacter::~PlayableCharacter() { }
		
	PlayableCharacter::PlayableCharacter(MemoryStream &sr) :m_character(sr), m_class_id(CharacterClass::defaultId()) {
		sr >> m_entity_ref;
		m_class_id = decodeInt(sr);
		ASSERT(CharacterClass::isValidId(m_class_id));
	}

	void PlayableCharacter::save(MemoryStream &sr) const {
		sr << m_character << m_entity_ref;
		encodeInt(sr, m_class_id);
	}

	void PlayableCharacter::load(MemoryStream &sr) {
		*this = PlayableCharacter(sr);
	}
		
	bool PlayableCharacter::operator==(const PlayableCharacter &rhs) const {
		return m_character == rhs.m_character && m_class_id == rhs.m_class_id && m_entity_ref == rhs.m_entity_ref;
	}

}
