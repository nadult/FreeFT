/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/weapon.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "audio/device.h"

using namespace gfx;

namespace hud {

	HudWeapon::HudWeapon(const FRect &target_rect)
		:HudWidget(target_rect), m_attack_mode(AttackMode::undefined), m_ammo_count(0) { }
		
	void HudWeapon::draw() const {
		HudWidget::draw();
		FRect rect = this->rect();

		if(!m_weapon.isDummy()) {
			FRect uv_rect;
			gfx::PTexture texture = m_weapon.guiImage(false, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (int2)(rect.center() - size / 2);
			texture->bind();
			drawQuad(FRect(pos, pos + size), uv_rect);

			//TODO: print current attack mode
			if(m_weapon.proto().max_ammo) {
				TextFormatter fmt(256);
				fmt("%d/%d", m_ammo_count, m_weapon.proto().max_ammo);
				IRect extents = m_font->evalExtents(fmt.text());
				m_font->drawShadowed(int2(rect.max.x - extents.width(), rect.min.y), m_style.focus_color, Color::black, "%s", fmt.text());
			}
		}
	}

}
