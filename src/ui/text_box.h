/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

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

