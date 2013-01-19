/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef UI_TEXT_BOX_H
#define UI_TEXT_BOX_H

#include "ui/window.h"
#include "gfx/font.h"


namespace ui {

	class TextBox: public Window
	{
	public:
		TextBox(const IRect &rect, const char *text, bool is_centered = true, Color col = Color::transparent);
		virtual const char *typeName() const { return "TextBox"; }

		void setFont(const char *font_name);
		void setText(const char *text);
		void drawContents() const;

	private:
		gfx::PFont m_font;
		string m_text;
		IRect m_text_extents;
		bool m_is_centered;
	};

	typedef Ptr<TextBox> PTextBox;

}

#endif

