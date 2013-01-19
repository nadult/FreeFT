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

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui/window.h"
#include "gfx/font.h"

namespace ui
{

	class Button: public Window
	{
	public:
		Button(IRect rect, const char *text, int id = 0);
		virtual const char *typeName() const { return "Button"; }

		virtual void drawContents() const;
		virtual bool onMouseDrag(int2, int2, int key, int is_final);
		virtual void setText(const char *text);

		void enable(bool);
		int id() const { return m_id; }

	protected:
		int m_id;
		bool m_is_enabled;
		bool m_mouse_press;
		IRect m_text_extents;
		string m_text;
		gfx::PFont m_font;
	};

	typedef Ptr<Button> PButton;


}


#endif
