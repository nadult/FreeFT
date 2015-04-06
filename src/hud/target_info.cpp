/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/target_info.h"
#include "hud/char_icon.h"
#include "game/actor.h"
#include "game/world.h"

using namespace gfx;

namespace hud {

	namespace {
		const float2 s_char_icon_size(52.0f, 70.0f);

		Color s_health_colors[HealthStatus::count] = {
			Color::white,
			Color::green,
			Color::yellow,
			Color(255, 150, 0),
			Color::red,
			Color::black
		};
	}

	HudTargetInfo::HudTargetInfo(const FRect &target_rect)
		:HudLayer(target_rect), m_hit_chance(0.0f), m_health(1.0f), m_kills(0), m_deaths(0) {
		m_anim_speed = 10.0f;
		FRect rect = this->rect();

		FRect icon_rect(s_char_icon_size);
		icon_rect += float2(rect.width() - icon_rect.width() - layer_spacing, rect.center().y - icon_rect.height() * 0.5f);
		m_char_icon = new HudCharIcon(icon_rect);
		m_char_icon->setEnabled(true, false);
		attach(m_char_icon.get());
	}
		
	HudTargetInfo::~HudTargetInfo() { }
		
	void HudTargetInfo::setCharacter(PCharacter character) {
		m_char_icon->setCharacter(character);
	}

	void HudTargetInfo::setStats(const GameClientStats &stats) {
		m_kills = stats.kills;
		m_deaths = stats.deaths;
	}
		
	void HudTargetInfo::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);
		m_char_icon->setVisible(m_is_visible);
	}

	void HudTargetInfo::onDraw() const {
		HudLayer::onDraw();

		HealthStatus::Type health = HealthStatus::fromHPPercentage(m_health);
		const char *health_desc = HealthStatus::toString(health);

		Color text_color = mulAlpha(Color::white, alpha());
		Color shadow_color = mulAlpha(Color::black, alpha());
		Color health_color = mulAlpha(s_health_colors[health], alpha());
		
		FRect font_rect = rect() + float2(layer_spacing, 0.0f);
		font_rect.max.y = font_rect.min.y + font_rect.height() / 4.0f;
		float2 offset = float2(0.0f, font_rect.height());
		m_font->draw(font_rect, {text_color, shadow_color, HAlign::left, VAlign::center}, m_name);

		font_rect += offset;
		m_font->draw(font_rect, {health_color, shadow_color, HAlign::left, VAlign::center}, health_desc);
		
		font_rect += offset;
		m_font->draw(font_rect, {text_color, shadow_color, HAlign::left, VAlign::center}, format("k/d: %d/%d", m_kills, m_deaths));

		font_rect += offset;
		m_font->draw(font_rect, {text_color, shadow_color, HAlign::left, VAlign::center}, format("Hit chance: %.0f%%", m_hit_chance * 100.0f));
	}

}
