// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/base.h"
#include "game/entity.h"
#include "game/inventory.h"

namespace game {

class Character : public std::enable_shared_from_this<Character> {
  public:
	static constexpr int max_name_size = 14, max_icon_name_size = 32;

	// TODO: different icons match different protos
	Character(const string &name, const string &icon_name, const string &proto_name);
	explicit Character(MemoryStream &);

	void save(MemoryStream &) const;
	void load(MemoryStream &);

	const string &name() const { return m_name; }
	const Proto &proto() const { return getProto(m_proto_idx); }

	PTexture icon() const;
	static PTexture emptyIcon();

	static const vector<pair<ProtoIndex, string>> findIcons();
	static const vector<string> findIcons(const string &proto_name);

	bool operator==(const Character &rhs) const;

  private:
	void validate();

	string m_name;
	string m_icon_name;
	ProtoIndex m_proto_idx;
};

class CharacterClass : public std::enable_shared_from_this<CharacterClass> {
  public:
	CharacterClass(const CharacterClass &) = default;
	CharacterClass(XmlNode node, int id);

	int id() const { return m_id; }
	int tier() const { return m_tier; }
	const string &name() const { return m_name; }

	const ActorInventory inventory(bool equip) const;
	bool isValidForActor(const string &proto_name) const;

	bool operator==(const CharacterClass &rhs) const { return m_id == rhs.m_id; }

	static void loadAll();
	static const CharacterClass &get(int id);
	static int count();
	static bool isValidId(int id) { return id >= 0 && id < count(); }
	static int defaultId() { return 0; }

  private:
	vector<string> m_proto_names;
	ActorInventory m_inventory;
	string m_name;
	int m_tier, m_id;
};

class PlayableCharacter : public std::enable_shared_from_this<PlayableCharacter> {
  public:
	PlayableCharacter(const Character &character, int class_id);
	explicit PlayableCharacter(MemoryStream &);
	~PlayableCharacter();

	void save(MemoryStream &) const;
	void load(MemoryStream &);

	void setClassId(int id) { m_class_id = id; }
	int classId() const { return m_class_id; }
	const CharacterClass &characterClass() const { return CharacterClass::get(m_class_id); }

	void setEntityRef(EntityRef ref) { m_entity_ref = ref; }
	EntityRef entityRef() const { return m_entity_ref; }

	const Character &character() const { return m_character; }
	bool operator==(const PlayableCharacter &) const;

  private:
	Character m_character;
	EntityRef m_entity_ref;
	int m_class_id;
};

}
