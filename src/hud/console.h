/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CONSOLE_H
#define HUD_CONSOLE_H

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
		bool onInput(const io::InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onDraw() const override;

		Ptr<HudEditBox> m_edit_box;
		vector<string> m_commands;
	};

}

#endif
