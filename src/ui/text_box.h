#ifndef UI_TEXT_BOX_H
#define UI_TEXT_BOX_H

#include "ui/window.h"
#include "gfx/font.h"


namespace ui {

	class TextBox: public Window
	{
	public:
		TextBox(const IRect &rect, const char *text, bool is_centered = true, Color col = Color::transparent);
		void setText(const char *text);
		void drawContents() const;

	private:
		gfx::PFont m_font;
		string m_text;
		int2 m_text_size;
		bool m_is_centered;
	};

}

#endif

