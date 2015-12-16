/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "ui/window.h"

namespace ui
{

	class Button: public Window
	{
	public:
		Button(IRect rect, const char *text, int id = 0);
		const char *typeName() const override { return "Button"; }

		void drawContents(Renderer2D&) const override;
		bool onMouseDrag(const InputState&, int2, int2, int key, int is_final) override;
		virtual void setText(const char *text);

		void enable(bool);
		int id() const { return m_id; }

	protected:
		int m_id;
		bool m_is_enabled;
		bool m_mouse_press;
		IRect m_text_extents;
		string m_text;
		PFont m_font;
	};

	using PButton = shared_ptr<Button>;

}


#endif
