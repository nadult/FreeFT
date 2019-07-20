// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef HUD_EDIT_BOX_H
#define HUD_EDIT_BOX_H

#include "hud/button.h"

namespace hud {

	class HudEditBox: public HudButton {
	public:
		enum EditMode {
			mode_normal,
			mode_console,
			mode_nick,
			mode_locase_nick,
		};

		HudEditBox(const FRect &rect, int max_size, EditMode mode = mode_normal, int id = 0);

		void setText(const string&);
		const string &text() const { return m_text; }

	protected:
		void onUpdate(double time_diff) override;
		void onDraw(Renderer2D&) const override;
		bool onInput(const InputEvent&) override;
		void onInputFocus(bool is_focused) override;
		void setCursorPos(const float2 &rect_pos);
		const FRect evalExtents(const string &text) const;
		bool isValidChar(int key);
	
		string m_text, m_old_text;
		int m_max_size, m_cursor_pos;
		EditMode m_mode;
		double m_show_time;
	};

}

#endif
