/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/class.h"
#include "game/character.h"
#include "game/inventory.h"

#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include <algorithm>

using namespace gfx;

namespace hud {
	
	namespace {
		const float2 s_button_size(15, 15);
		const float s_bottom_size = 30.0f;
		const float s_item_height = 65.0f;
		const float s_min_item_width = 50.0f;
		int m_max_buttons = 4;
	}
		
	HudClassButton::HudClassButton(const FRect &rect) :HudButton(rect), m_class_id(-1) { }
		
	void HudClassButton::setId(int class_id) {
		DASSERT(class_id == -1 || CharacterClass::isValidId(class_id));
		m_class_id = class_id;
	}

	void HudClassButton::onDraw() const {
		HudButton::onDraw();
		FRect rect = this->rect();

		if(m_class_id != -1) {
			ActorInventory inv = CharacterClass(m_class_id).inventory(false);
			float2 pos(spacing, rect.center().y);

			for(int n = 0; n < inv.size(); n++) {
				FRect uv_rect;
				gfx::PTexture texture = inv[n].item.guiImage(true, uv_rect);
				float2 size(texture->width() * uv_rect.width(), texture->height() * uv_rect.height());

				FRect irect(pos.x, rect.min.y, pos.x + max(s_min_item_width, size.x), rect.max.y);
				if(irect.max.x > rect.width())
					break;

				texture->bind();
				drawQuad(FRect(irect.center() - size * 0.5f, irect.center() + size * 0.5f), uv_rect);

				if(inv[n].count > 1) {
					FRect trect = irect;
					trect.max.y -= 5.0;
					m_font->draw(trect, {enabledColor(), enabledShadowColor(), HAlign::right, VAlign::bottom}, format("%d", inv[n].count));
				}
				
				pos.x += irect.width() + spacing;
			}
		}
	}
		
	Color HudClassButton::backgroundColor() const {
		return lerp(HudButton::backgroundColor(), Color::white, m_enabled_time * 0.5f);
	}

	HudClass::HudClass(PWorld world, const FRect &target_rect)
		:HudLayer(target_rect), m_offset(0), m_selected_id(-1) {

		m_class_count = CharacterClass::count();
		for(int n = 0; n < m_max_buttons; n++) {
			float diff = s_item_height + spacing * 2;
			float2 pos(HudButton::spacing, HudButton::spacing + (s_item_height + HudButton::spacing) * n);
			FRect rect(pos, pos + float2(target_rect.width() - HudButton::spacing * 2, s_item_height));
			Ptr<HudClassButton> button(new HudClassButton(rect));
			attach(button.get());
			m_buttons.push_back(std::move(button));
		}

		m_button_up = new HudButton(FRect(s_button_size));
		m_button_up->setIcon(HudIcon::up_arrow);
		m_button_up->setAccelerator(Key::pageup);

		m_button_down = new HudButton(FRect(s_button_size));
		m_button_down->setIcon(HudIcon::down_arrow);
		m_button_down->setAccelerator(Key::pagedown);

		attach(m_button_up.get());
		attach(m_button_down.get());
	}
		
	HudClass::~HudClass() { }

	void HudClass::onUpdate(double time_diff) {
/*		float bottom_line = rect().height() - s_bottom_size;

		int max_offset = (m_class_count + m_max_buttons - 1) / m_max_buttons - 1;
		int over_item = -1;

		for(int n = 0; n < (int)m_buttons.size(); n++) {
			int id = n + m_offset;
			if(id >= m_class_count)
				id = -1;

			m_buttons[n]->setVisible(id != -1);
			m_buttons[n]->setFocus(m_selected_id == id && id != -1);
			m_buttons[n]->setId(id);
			
			if(m_buttons[n]->isMouseOver(mouse_pos))
				over_item = n;
		}

		{
			HudStyle button_style = m_style;
			button_style.border_offset *= 0.5f;
			m_button_up->setStyle(button_style);
			m_button_down->setStyle(button_style);
			m_button_up  ->setPos(float2(rect().width() - HudWidget::spacing * 2 - s_button_size.x * 2, bottom_line + 5.0f));
			m_button_down->setPos(float2(rect().width() - HudWidget::spacing * 1 - s_button_size.x * 1, bottom_line + 5.0f));
			m_button_up->setVisible(m_offset > 0);
			m_button_down->setVisible(m_offset < max_offset);

			m_button_up->setFocus(m_button_up->rect().isInside(mouse_pos) && isMouseKeyPressed(0));
			m_button_down->setFocus(m_button_down->rect().isInside(mouse_pos) && isMouseKeyPressed(0));

			if(m_button_up->isPressed(mouse_pos) && m_offset > 0) {
				playSound(HudSound::button);
				m_offset--;
			}
			if(m_button_down->isPressed(mouse_pos) && m_offset < max_offset) {
				playSound(HudSound::button);
				m_offset++;
			}
		}

		if(is_active && over_item != -1) {
			if(m_buttons[over_item]->isPressed(mouse_pos, 0)) {
				m_selected_id = over_item;
				playSound(HudSound::button);
			}
		}*/
	}

}
