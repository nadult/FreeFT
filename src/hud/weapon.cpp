// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/weapon.h"

#include "game/actor.h"
#include "game/world.h"
#include "gfx/drawing.h"
#include <fwk/gfx/font.h>

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
			out.addFilledRect(FRect(pos, pos + size), uv_rect, {texture, ColorId::white});

			//TODO: print current attack mode
			if(m_weapon.proto().max_ammo) {
				//TODO: alpha for shadow color
				FontStyle style{m_style.enabled_color, ColorId::black, HAlign::right, VAlign::top};
				m_font->draw(out, rect, style, format("%/%", m_ammo_count, m_weapon.proto().max_ammo));
			}
		}
	}

}
