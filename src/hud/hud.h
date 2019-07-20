// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef HUD_HUD_H
#define HUD_HUD_H

#include "game/base.h"
#include "game/entity.h"
#include "hud/widget.h"

//TODO: add help layer

namespace hud
{

	class Hud: public HudWidget {
	public:
		Hud(game::PWorld world, const int2 &window_size);
		~Hud();

		void setVisible(bool is_visible, bool animate = true) override;
		bool isVisible() const;

		void showLayer(int layer_id);
		void setWorld(game::PWorld);
		void setPCController(game::PPCController);
		HudMainPanel *mainPanel() { return m_main_panel.get(); }

		int exitRequested() const { return m_exit_value; }

	protected:
		bool onInput(const InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onUpdate(double time_diff) override;
		void onLayout() override;

	private:
		int m_selected_layer;
		int m_exit_value;
		PHudMainPanel m_main_panel;
		PHudLayer m_layers[layer_count];
	};

}

#endif
