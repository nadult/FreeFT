/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/character.h"
#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include <algorithm>

using namespace gfx;

namespace hud {
	
	namespace {
		const float2 s_item_size(70, 65);
		const float2 s_item_desc_size(250, 300);
		const float2 s_button_size(15, 15);
		const float2 s_hud_char_icon_size(75, 100);
		const float2 s_name_size(240, 24);
		const float s_bottom_size = 30.0f;

		const int2 s_grid_size(4, 10);
	}
		
	HudCharacter::HudCharacter(PWorld world, const FRect &target_rect)
		:HudLayer(world, target_rect), m_icon_id(-1) {

		float2 pos(spacing, spacing);
		m_icon_box = new HudCharIcon(FRect(s_hud_char_icon_size) + pos);
		
		pos.x += s_hud_char_icon_size.x + spacing;
		m_name_edit_box = new HudEditBox(FRect(s_name_size) + pos, 20);
		m_name_edit_box->setText("Name: ");

		pos.y += s_hud_char_icon_size.y + HudButton::spacing;
		pos.x = m_icon_box->rect().center().x - s_button_size.x - spacing * 0.5f;

		m_button_up = new HudButton(FRect(s_button_size) + pos);
		m_button_up->setIcon(HudIcon::up_arrow);
		m_button_up->setAccelerator(Key::pageup);
		pos.x += s_button_size.x + spacing;

		m_button_down = new HudButton(FRect(s_button_size) + pos);
		m_button_down->setIcon(HudIcon::down_arrow);
		m_button_down->setAccelerator(Key::pagedown);

		m_icons = game::Character::findIcons();

		attach(m_button_up.get());
		attach(m_button_down.get());
		attach(m_icon_box.get());
		attach(m_name_edit_box.get());
	}
		
	HudCharacter::~HudCharacter() { }
		
	void HudCharacter::onUpdate(double time_diff) {
		/*float bottom_line = rect().height() - s_bottom_size;

		int num_icons = 0;
		ProtoIndex index = findProto("male", ProtoId::actor);

		if(m_icon_id < 0 || m_icon_id >= (int)m_icons.size() || m_icons[m_icon_id].first != index)
			m_icon_id = -1;
		if(m_icon_id == -1) {
			for(int n = 0; n < (int)m_icons.size(); n++)
				if(m_icons[n].first == index)
					m_icon_id = n;
		}

		int max_row_offset = 0;

		if(m_button_up->isPressed(mouse_pos)) {
			playSound(HudSound::button);
			int old_id = m_icon_id++;
			while(m_icon_id != old_id) {
				if(m_icon_id >= (int)m_icons.size()) {
					m_icon_id = 0;
					continue;
				}
				if(m_icons[m_icon_id].first == index)
					break;
				m_icon_id++;
			}
		}
		if(m_button_down->isPressed(mouse_pos)) {
			playSound(HudSound::button);
			int old_id = m_icon_id--;
			while(m_icon_id != old_id) {
				if(m_icon_id == -1) {
					m_icon_id = (int)m_icons.size() - 1;
					continue;
				}
				if(m_icons[m_icon_id].first == index)
					break;
				m_icon_id--;
			}
		}

		m_character = new Character("unnamed", m_icon_id == -1? "" : m_icons[m_icon_id].second, getProto(index).id);
		m_icon_box->setCharacter(m_character);
		//TODO: update icon_box afterwards?*/
	}

}
