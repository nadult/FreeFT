// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/target_info.h"

#include "game/actor.h"
#include "game/world.h"
#include "hud/char_icon.h"
#include <fwk/gfx/font.h>

namespace hud {

namespace {
	const float2 s_char_icon_size(52.0f, 70.0f);

	static const EnumMap<HealthStatus, Color> s_health_colors = {
		{ColorId::white, ColorId::green, ColorId::yellow, Color(255, 150, 0), ColorId::red,
		 ColorId::black}};
}

HudTargetInfo::HudTargetInfo(const FRect &target_rect)
	: HudLayer(target_rect), m_hit_chance(0.0f), m_health(1.0f), m_kills(0), m_deaths(0) {
	m_anim_speed = 10.0f;
	FRect rect = this->rect();

	FRect icon_rect(s_char_icon_size);
	icon_rect += float2(rect.width() - icon_rect.width() - layer_spacing,
						rect.center().y - icon_rect.height() * 0.5f);
	m_char_icon = make_shared<HudCharIcon>(icon_rect);
	m_char_icon->setEnabled(true, false);
	attach(m_char_icon);
}

HudTargetInfo::~HudTargetInfo() {}

void HudTargetInfo::setCharacter(PCharacter character) { m_char_icon->setCharacter(character); }

void HudTargetInfo::setStats(const GameClientStats &stats) {
	m_kills = stats.kills;
	m_deaths = stats.deaths;
}

void HudTargetInfo::onUpdate(double time_diff) {
	HudLayer::onUpdate(time_diff);
	m_char_icon->setVisible(m_is_visible);
}

void HudTargetInfo::onDraw(Canvas2D &out) const {
	HudLayer::onDraw(out);

	HealthStatus health = healthStatusFromHP(m_health);
	const char *health_desc = toString(health);

	Color text_color = (Color)mulAlpha(ColorId::white, alpha());
	Color shadow_color = (Color)mulAlpha(ColorId::black, alpha());
	Color health_color = (Color)mulAlpha(s_health_colors[health], alpha());

	FRect font_rect = rect() + float2(layer_spacing, 0.0f);
	font_rect = {font_rect.min(), {font_rect.ex(), font_rect.y() + font_rect.height() / 4.0f}};
	float2 offset = float2(0.0f, font_rect.height());
	m_font->draw(out, font_rect, {text_color, shadow_color, HAlign::left, VAlign::center}, m_name);

	font_rect += offset;
	m_font->draw(out, font_rect, {health_color, shadow_color, HAlign::left, VAlign::center},
				 health_desc);

	font_rect += offset;
	m_font->draw(out, font_rect, {text_color, shadow_color, HAlign::left, VAlign::center},
				 format("k/d: %/%", m_kills, m_deaths));

	font_rect += offset;
	m_font->draw(out, font_rect, {text_color, shadow_color, HAlign::left, VAlign::center},
				 stdFormat("Hit chance: %.0f%%", m_hit_chance * 100.0f));
}

}
