/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_HUD_H
#define HUD_HUD_H

#include "game/base.h"
#include "game/entity.h"
#include "hud/widget.h"

namespace hud
{

	class Hud: public HudWidget {
	public:
		Hud(game::PWorld world);
		~Hud();

		void setVisible(bool is_visible, bool animate = true);
		bool isVisible() const;

		void setPC(game::PPlayableCharacter);

	protected:
		bool onInput(const io::InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onUpdate(double time_diff) override;

	private:
		int m_selected_layer;
		Ptr<HudMainPanel> m_main_panel;
		PHudLayer m_layers[layer_count];
	};

}

#endif
