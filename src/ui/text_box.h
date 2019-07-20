// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef UI_TEXT_BOX_H
#define UI_TEXT_BOX_H

#include "ui/window.h"


namespace ui {

	class TextBox: public Window
	{
	public:
		TextBox(const IRect &rect, const char *text, bool is_centered = true, Color col = ColorId::transparent);
		const char *typeName() const override { return "TextBox"; }

		void setFont(const char *font_name);
		void setText(const char *text);
		void drawContents(Renderer2D&) const override;

	private:
		PFont m_font;
		string m_text;
		IRect m_text_extents;
		bool m_is_centered;
	};

	using PTextBox = shared_ptr<TextBox>;

}

#endif

