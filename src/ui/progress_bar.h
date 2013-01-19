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

#ifndef UI_PROGRESS_BAR
#define UI_PROGRESS_BAR

#include "ui/window.h"
#include "gfx/font.h"

namespace ui {


	class ProgressBar: public Window {
	public:
		ProgressBar(const IRect &rect, bool is_horizontal);
		virtual const char *typeName() const { return "ProgressBar"; }

		void setText(const char*);

		// range: <0; 1>
		void setBarSize(float size);
		// range: <0; 1>
		void setPos(float pos);

		float pos() const { return m_pos; }
		float barSize() const { return m_bar_size; }

		virtual void drawContents() const;
		virtual bool onMouseDrag(int2 start, int2 current, int key, int is_final);

	protected:
		float evalBarSize() const;
		IRect evalBarPos() const;

		string m_text;
		gfx::PFont m_font;
		float m_bar_size, m_pos, m_start_pos;
		bool m_mouse_press, m_mouse_over;
		bool m_is_horizontal;
	};

	typedef Ptr<ProgressBar> PProgressBar;

}


#endif
