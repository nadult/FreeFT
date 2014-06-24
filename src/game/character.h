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

		Character(const string &name, const string &icon_name);
		bool isValid() const;

		void save(Stream&) const;
		void load(Stream&);

		const string &name() const { return m_name; }
		gfx::PTexture icon() const;
		static gfx::PTexture defaultIcon();

	private:
		string m_name;
		string m_icon_name;
	};

}

#endif
