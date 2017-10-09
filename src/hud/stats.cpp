/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/stats.h"
#include "hud/grid.h"
#include "game/pc_controller.h"
#include "game/world.h"
#include "game/game_mode.h"

using namespace gfx;

namespace hud {

	HudStats::HudStats(const FRect &target_rect)
		:HudLayer(target_rect) {
		setTitle("Statistics:");

		FRect sub_rect(rect().size());
		sub_rect = {{sub_rect.x(), sub_rect.y() + topOffset()}, sub_rect.max()};
		sub_rect = sub_rect.inset(float2(layer_spacing, layer_spacing));
		sub_rect = {sub_rect.min(), {sub_rect.ex(), sub_rect.y() + 300.0f}};

		m_grid = make_shared<HudGrid>(sub_rect);
		m_grid->addColumn("Nick name", 120.0f);
		m_grid->addColumn("Kills", 40.0f);
		m_grid->addColumn("Deaths", 40.0f);

		attach(m_grid);
	}
		
	HudStats::~HudStats() { }
	
	bool HudStats::canShow() const {
		return m_pc_controller && m_pc_controller->world().isClient();
	}

	void HudStats::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);
		updateData();
	}
		
	void HudStats::updateData() {
		m_grid->clearRows();

		if(m_pc_controller) {
			const GameModeClient *game_mode = dynamic_cast<const GameModeClient*>(m_pc_controller->world().gameMode());
			if(game_mode) {
				const auto &stats = game_mode->stats();

				int counter = 0;
				for(auto &stat : stats) {
					m_grid->addRow(counter);
					m_grid->setCell(counter, 0, stat.nick_name);
					m_grid->setCell(counter, 1, format("%", stat.kills));
					m_grid->setCell(counter, 2, format("%", stat.deaths));
					counter++;
				}
			}
		}
	}

	void HudStats::onDraw(Renderer2D &out) const {
		HudLayer::onDraw(out);
	}
		
}
