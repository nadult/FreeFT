/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef UI_EDIT_BOX_H
#define UI_EDIT_BOX_H

#include "ui/window.h"
#include "gfx/font.h"


namespace ui {

	class EditBox: public Window
	{
	public:
		EditBox(const IRect &rect, int max_size, Color col = Color::transparent);
		virtual const char *typeName() const { return "EditBox"; }

		void setText(const char *text);
		const char *text() const { return m_text.c_str(); }

		void drawContents() const;
		void onInput(int2);
		bool onEvent(const Event&);

	private:
		void onKey(int key);
		void setCursorPos(int2);

		int m_max_size;
		int m_last_key;
		double m_key_down_time;
		double m_on_key_time;

		gfx::PFont m_font;
		string m_text, m_old_text;
		int m_cursor_pos;
		bool m_is_editing;
	};

	typedef Ptr<EditBox> PEditBox;

}

#endif

