/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/char_icon.h"

#include "game/actor.h"
#include "game/world.h"
#include "gfx/drawing.h"
#include <fwk/gfx/font.h>

namespace hud {

	HudCharIcon::HudCharIcon(const FRect &target_rect)
		:HudButton(target_rect), m_current_hp(0), m_max_hp(0) {
		setClickSound(HudSound::none);
	}
		
	HudCharIcon::~HudCharIcon() { }

	void HudCharIcon::onDraw(Renderer2D &out) const {
		HudButton::onDraw(out);
		FRect rect = this->rect();
	
		PTexture icon = m_character? m_character->icon() : Character::emptyIcon();
		//TODO: use mipmapped textures

		float2 icon_size(icon->size());
		float scale = min(1.0f, 1.0f / max(icon_size.x / rect.width(), icon_size.y / rect.height()));
		icon_size = icon_size * scale;
		float2 pos = rect.center() - icon_size * 0.5f;

		float hp_value = m_max_hp? clamp(float(m_current_hp) / m_max_hp, 0.0f, 1.0f) : 1.0f;
		FColor color(lerp(FColor(ColorId::red), FColor(ColorId::green), hp_value));
		color = lerp(color, FColor(m_style.enabled_color), 0.5f);

		out.addFilledRect(FRect(pos, pos + icon_size), {icon, mulAlpha(color, alpha())});

		if(m_max_hp) {
			auto text = hp_value <= 0.0f? "DEAD" : format("%/%", m_current_hp, m_max_hp);
			m_font->draw(out, rect, {m_style.enabled_color, ColorId::black, HAlign::right, VAlign::top}, text);
		}
	}

}
