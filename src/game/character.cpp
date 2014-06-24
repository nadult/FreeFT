/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/character.h"
#include "gfx/device.h"

namespace game {

	Character::Character(const string &name, const string &icon_name)
   		:m_name(name), m_icon_name(icon_name) {
		ASSERT(isValid());
	}

	bool Character::isValid() const {
		return	m_name.size() <= max_name_size &&
				m_icon_name.size() < max_icon_name_size;
	}
		
	void Character::save(Stream &sr) const {
		sr << m_name << m_icon_name;
	}

	void Character::load(Stream &sr) {
		sr >> m_name >> m_icon_name;
		ASSERT(isValid());
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

}
