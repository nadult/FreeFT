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
		int s_max_buttons = 4;
	}
		
	HudClassButton::HudClassButton(const FRect &rect)
		:HudRadioButton(rect, -1, 1) { }
		
	void HudClassButton::onDraw() const {
		HudButton::onDraw();
		FRect rect = this->rect();

		if(m_id != -1) {
			CharacterClass char_class(m_id);
			ActorInventory inv = char_class.inventory(false);
			float2 pos(spacing + 30.0f, rect.center().y);

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
					m_font->draw(trect, {textColor(), textShadowColor(), HAlign::right, VAlign::bottom}, format("%d", inv[n].count));
				}
				
				pos.x += irect.width() + spacing;
			}

			m_font->draw(rect, {textColor(), textShadowColor(), HAlign::left, VAlign::top}, char_class.name());
		}
	}
		
	Color HudClassButton::backgroundColor() const {
		return lerp(HudButton::backgroundColor(), Color::white, m_enabled_time * 0.5f);
	}

	HudClass::HudClass(const FRect &target_rect)
		:HudLayer(target_rect), m_offset(0), m_selected_id(-1) {

		m_class_count = CharacterClass::count();
		for(int n = 0; n < s_max_buttons; n++) {
			float diff = s_item_height + spacing * 2;
			float2 pos(HudButton::spacing, HudButton::spacing + (s_item_height + HudButton::spacing) * n);
			FRect rect(pos, pos + float2(target_rect.width() - HudButton::spacing * 2, s_item_height));
			Ptr<HudClassButton> button(new HudClassButton(rect));
			attach(button.get());
			m_buttons.push_back(std::move(button));
		}

		m_button_up = new HudClickButton(FRect(s_button_size));
		m_button_up->setIcon(HudIcon::up_arrow);
		m_button_up->setAccelerator(Key::pageup);
		m_button_up->setButtonStyle(HudButtonStyle::small);

		m_button_down = new HudClickButton(FRect(s_button_size));
		m_button_down->setIcon(HudIcon::down_arrow);
		m_button_down->setAccelerator(Key::pagedown);
		m_button_down->setButtonStyle(HudButtonStyle::small);

		attach(m_button_up.get());
		attach(m_button_down.get());
	}
		
	HudClass::~HudClass() { }
		
	bool HudClass::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::button_clicked) {
			int max_offset = (m_class_count + s_max_buttons - 1) / s_max_buttons - 1;

			if(m_button_up == event.source && m_offset > 0)
				m_offset--;
			if(m_button_down == event.source && m_offset < max_offset)
				m_offset++;
			if(isOneOf(event.source, m_buttons)) {
				HudButton *button = dynamic_cast<HudButton*>(event.source);
				m_selected_id = button->id();
			}

			needsLayout();

			return true;
		}

		return false;
	}
		
	void HudClass::onLayout() {
		int max_offset = (m_class_count + s_max_buttons - 1) / s_max_buttons - 1;

		float bottom = 0.0f;
		for(int n = 0; n < (int)m_buttons.size(); n++) {
			int id = n + m_offset;
			if(id >= m_class_count)
				id = -1;

			m_buttons[n]->setVisible(id != -1, false);
			m_buttons[n]->setEnabled(m_selected_id == id && id != -1);
			m_buttons[n]->setId(id);
			
			if(id != -1)
				bottom = max(bottom, m_buttons[n]->rect().max.y + spacing);
		}

		m_button_up  ->setPos(float2(rect().width() - spacing * 2 - s_button_size.x * 2, bottom + 5.0f));
		m_button_down->setPos(float2(rect().width() - spacing * 1 - s_button_size.x * 1, bottom + 5.0f));
		m_button_up->setGreyed(m_offset == 0);
		m_button_down->setGreyed(m_offset >= max_offset);
	}

	void HudClass::onUpdate(double time_diff) {
	}

}
