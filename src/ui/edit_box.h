/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef UI_EDIT_BOX_H
#define UI_EDIT_BOX_H

#include "ui/window.h"


namespace ui {

	class EditBox: public Window
	{
	public:
		EditBox(const IRect &rect, int max_size, StringRef label = StringRef(), Color col = Color::transparent);
		const char *typeName() const override { return "EditBox"; }

		void setText(wstring);
		const wstring &text() const { return m_text; }
		void reset(bool is_editing);

		void drawContents(Renderer2D&) const override;
		void onInput(const InputState&) override;
		bool onEvent(const Event&) override;
		//TODO: what if text character is not available in the font?

	private:
		void setCursorPos(int2);

		PFont m_font;
		wstring m_text, m_old_text;
		wstring m_label;
		int m_cursor_pos;
		int m_max_size;
		bool m_is_editing;
	};

	using PEditBox = shared_ptr<EditBox>;

}

#endif

