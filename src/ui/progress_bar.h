/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

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
