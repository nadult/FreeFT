/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "ui/image_button.h"
#include "audio/device.h"
#include "gfx/drawing.h"

namespace ui
{
	ImageButtonProto::ImageButtonProto(const char *back_tex, const char *up_tex, const char *down_tex, const char *font_name, FRect text_area) {
		DASSERT(up_tex && down_tex);

		if(back_tex)
			back = res::guiTextures()[back_tex];

		up = res::guiTextures()[up_tex];
		down = res::guiTextures()[down_tex];
		if(font_name)
			font = res::getFont(font_name);

		rect = IRect({0, 0}, back? back->size() : vmax(up->size(), down->size()));
		text_rect = text_area.empty()? IRect() :
			IRect(	lerp(float(rect.min.x), float(rect.max.x), text_area.min.x),
					lerp(float(rect.min.y), float(rect.max.y), text_area.min.y),
					lerp(float(rect.min.x), float(rect.max.x), text_area.max.x),
					lerp(float(rect.min.y), float(rect.max.y), text_area.max.y));
	}

	ImageButton::ImageButton(const int2 &pos, ImageButtonProto proto, const char *text, Mode mode, int id)
		:Window(IRect(pos, pos + proto.rect.size()), ColorId::transparent), m_proto(std::move(proto)), m_id(id), m_mode(mode) {
		setBackground(m_proto.back);
		setText(text);

		m_is_enabled = true;
		m_is_pressed = false;
		m_mouse_press = false;
	}

	void ImageButton::setText(const char *text) {
		if(m_proto.font) {
			DASSERT(text);

			m_text = text;
			m_text_extents = m_proto.font->evalExtents(text);
			m_text_extents.min.y = 0;
			m_text_extents.max.y = m_proto.font->lineHeight();
		}
	}

	void ImageButton::drawContents(Renderer2D &out) const {
		bool is_pressed =	m_mode == mode_toggle? m_is_pressed ^ m_mouse_press :
							m_mode == mode_toggle_on? m_is_pressed || m_mouse_press : m_mouse_press;

		if(is_pressed)
			out.addFilledRect(IRect(m_proto.down->size()), m_proto.down);
		else
			out.addFilledRect(IRect(m_proto.up->size()), m_proto.up);

		if(m_proto.font) {
			int2 rect_center = size() / 2;
			int2 pos(m_proto.text_rect.min.x - 1, m_proto.text_rect.center().y - m_text_extents.height() / 2 - 1);

			if(m_mouse_press)
				pos += int2(2, 2);
			m_proto.font->draw(out, (float2)pos, {m_is_enabled? Color(255, 200, 0) : ColorId::gray, ColorId::black}, m_text);
		}
	}

	bool ImageButton::onMouseDrag(const InputState&, int2 start, int2 current, int key, int is_final) {
		if(key == 0 && !m_mouse_press && !m_proto.sound_name.empty() && !(m_mode == mode_toggle_on && m_is_pressed))
			audio::playSound(m_proto.sound_name.c_str(), 1.0f);
		m_mouse_press = key == 0 && !is_final && m_is_enabled;

		if(key == 0 && m_is_enabled) {
			if(is_final == 1 && localRect().isInside(current)) {
				if(m_mode == mode_toggle)
					m_is_pressed ^= 1;
				else if(m_mode == mode_toggle_on)
					m_is_pressed = true;
				sendEvent(this, Event::button_clicked, m_id);
			}
			return true;
		}

		return false;
	}

	void ImageButton::enable(bool do_enable) {
		if(m_is_enabled && !do_enable)
			m_mouse_press = false;
		m_is_enabled = do_enable;
	}

}
