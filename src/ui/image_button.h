// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef UI_IMAGE_BUTTON_H
#define UI_IMAGE_BUTTON_H

#include "ui/window.h"

namespace ui
{

	struct ImageButtonProto {
		ImageButtonProto(const char *back_tex, const char *up_tex, const char *down_tex, const char *font_name, FRect text_area);

		IRect rect, text_rect;
		PTexture back, up, down;
		PFont font;
		string sound_name;
	};

	class ImageButton: public Window
	{
	public:
		enum Mode {
			mode_normal,
			mode_toggle,
			mode_toggle_on
		};

		ImageButton(const int2 &pos, ImageButtonProto proto, const char *text, Mode mode, int id = 0);
		const char *typeName() const override { return "ImageButton"; }

		void drawContents(Renderer2D&) const override;
		bool onMouseDrag(const InputState&, int2, int2, int key, int is_final) override;
		virtual void setText(const char *text);

		bool isPressed() const { return m_mode != mode_normal && m_is_pressed; }
		void press(bool is_pressed) { if(m_mode != mode_normal) m_is_pressed = is_pressed; }

		bool isEnabled() const { return m_is_enabled; }
		void enable(bool);

		void setId(int id) { m_id = id; }
		int id() const { return m_id; }

	protected:
		ImageButtonProto m_proto;
		IRect m_text_extents;
		string m_text;
		int m_id;
		
		const Mode m_mode;
		bool m_is_enabled;
		bool m_is_pressed;
		bool m_mouse_press;
	};

}


#endif
