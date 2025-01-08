// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/game_mode.h"
#include "hud/layer.h"

namespace hud {

class HudTargetInfo : public HudLayer {
  public:
	HudTargetInfo(const FRect &target_rect);
	~HudTargetInfo();

	const FRect rect() const override { return m_rect; }
	float alpha() const override { return m_visible_time; }

	void setHitChance(float chance) { m_hit_chance = chance; }
	void setHealth(float health) { m_health = health; }
	void setCharacter(PCharacter);
	void setStats(const game::GameClientStats &stats);
	void setName(const string &name) { m_name = name; }

  protected:
	void onDraw(Canvas2D &) const override;
	void onUpdate(double) override;

	PHudCharIcon m_char_icon;
	string m_name;
	float m_hit_chance;
	float m_health;
	int m_kills, m_deaths;
};

}
