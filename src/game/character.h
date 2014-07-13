/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H

#include "game/base.h"

namespace game
{

	class Character: public RefCounter {
	public:
		enum {
			max_name_size = 20,
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

		bool operator==(const CharacterClass &rhs) const;

	private:
		int m_tier;
		int m_id;
	};

}

#endif
