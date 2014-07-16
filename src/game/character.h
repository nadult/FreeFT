/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H

#include "game/base.h"
#include "game/entity.h"

namespace game
{

	class Character: public RefCounter {
	public:
		enum {
			max_name_size = 14,
			max_icon_name_size = 32,
		};

		// TODO: different icons match different protos
		Character(const string &name, const string &icon_name, const string &proto_name);
		Character(Stream&);

		void save(Stream&) const;
		void load(Stream&);

		const string &name() const { return m_name; }
		const Proto &proto() const { return getProto(m_proto_idx); }

		gfx::PTexture icon() const;
		static gfx::PTexture emptyIcon();

		static const vector<pair<ProtoIndex, string>> findIcons();

		bool operator==(const Character &rhs) const;

	private:
		void validate();

		string m_name;
		string m_icon_name;
		ProtoIndex m_proto_idx;
	};

	class CharacterClass {
	public:
		CharacterClass(int class_id);

		static bool isValidId(int);
		static int count();

		int id() const { return m_id; }
		int tier() const { return m_tier; }
		const ActorInventory inventory(bool equip) const;
		const string name() const;

		bool operator==(const CharacterClass &rhs) const;

	private:
		int m_tier;
		int m_id;
	};

	class PlayableCharacter: public RefCounter {
	public:
		PlayableCharacter(const Character &character);
		~PlayableCharacter();
		
		void save(Stream&) const;
		void load(Stream&);

		void setCharacterClass(const CharacterClass &char_class) { m_class = char_class; }
		const CharacterClass &characterClass() const { return m_class; }

		void setEntityRef(EntityRef ref) { m_entity_ref = ref; }
		EntityRef entityRef() const { return m_entity_ref; }

		const Character &character() const { return m_character; }
		bool operator==(const PlayableCharacter&) const;

	private:
		Character m_character;
		CharacterClass m_class;
		EntityRef m_entity_ref;
	};

}

#endif
