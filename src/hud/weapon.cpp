/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/weapon.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/drawing.h"


namespace hud {

	HudWeapon::HudWeapon(const FRect &target_rect)
		:HudButton(target_rect) {
		setClickSound(HudSound::none);
	}
		
	void HudWeapon::onDraw(Renderer2D &out) const {
		HudButton::onDraw(out);
		FRect rect = this->rect();

		if(!m_weapon.isDummy()) {
			FRect uv_rect;
			auto texture = m_weapon.guiImage(false, uv_rect);
			float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

			float2 pos = (float2)(int2)(rect.center() - size / 2);
			out.addFilledRect(FRect(pos, pos + size), uv_rect, {texture, Color::white});

			//TODO: print current attack mode
			if(m_weapon.proto().max_ammo) {
				TextFormatter fmt;
				fmt("%d/%d", m_ammo_count, m_weapon.proto().max_ammo);
				//TODO: alpha for shadow color
				m_font->draw(out, rect, {m_style.enabled_color, Color::black, HAlign::right, VAlign::top}, fmt);
			}
		}
	}

}
