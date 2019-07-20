// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include "hud/layer.h"

namespace hud {

	class HudConsole: public HudLayer {
	public:
		HudConsole(const int2 &resolution);
		~HudConsole();

		const string getCommand();
		void setVisible(bool is_visible, bool animate = true) override;

	protected:
		void onUpdate(double time_diff) override;
		bool onInput(const InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onDraw(Renderer2D&) const override;

		PHudEditBox m_edit_box;
		vector<string> m_commands;
	};

}
