/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/char_icon.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"

using namespace gfx;

namespace hud {

	HudCharIcon::HudCharIcon(const FRect &target_rect)
		:HudWidget(target_rect), m_current_hp(0), m_max_hp(0) { }
		
	HudCharIcon::~HudCharIcon() { }

	void HudCharIcon::draw() const {
		HudWidget::draw();
		FRect rect = this->rect();
	
		PTexture icon = m_character? m_character->icon() : Character::defaultIcon();
		icon->bind();
		float2 pos = rect.center(), icon_size(icon->dimensions());
		pos -= icon_size * 0.5f;

		float hp_value = m_max_hp? clamp(float(m_current_hp) / m_max_hp, 0.0f, 1.0f) : 1.0f;
		Color color(lerp((float4)Color(Color::red), (float4)Color(Color::green), hp_value));
		color = lerp(color, m_style.focus_color, 0.5f);

		drawQuad(FRect(pos, pos + icon_size), color);

		if(m_max_hp) {
			TextFormatter fmt(256);
			fmt(hp_value <= 0.0f? "DEAD" : "%d/%d", m_current_hp, m_max_hp);
			IRect extents = m_font->evalExtents(fmt.text());

			float2 pos(rect.max.x - extents.width(), rect.min.y);
			m_font->drawShadowed((int2)pos, m_style.focus_color, Color::black, "%s", fmt.text());
		}
	}

}
