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
		const float2 s_scroll_button_size(15, 15);
		const float2 s_button_size(60, 15);
		const float2 s_hud_char_icon_size(75, 100);
		const float2 s_name_size(240, 24);
		const float s_bottom_size = 30.0f;

		const int2 s_grid_size(4, 10);
	}
		
	HudCharacter::HudCharacter(const FRect &target_rect)
		:HudLayer(target_rect) {

		setTitle("Character creation:");

		float2 pos(spacing, spacing + topOffset());
		m_icon_box = new HudCharIcon(FRect(s_hud_char_icon_size) + pos);
		
		pos.y += s_hud_char_icon_size.y + HudButton::spacing;
		pos.x = m_icon_box->rect().center().x - s_scroll_button_size.x - spacing * 0.5f;

		m_icon_prev = new HudClickButton(FRect(s_scroll_button_size) + pos);
		m_icon_prev->setIcon(HudIcon::up_arrow);
		m_icon_prev->setAccelerator(Key::pageup);
		m_icon_prev->setButtonStyle(HudButtonStyle::small);
		pos.x += s_scroll_button_size.x + spacing;

		m_icon_next = new HudClickButton(FRect(s_scroll_button_size) + pos);
		m_icon_next->setIcon(HudIcon::down_arrow);
		m_icon_next->setAccelerator(Key::pagedown);
		m_icon_next->setButtonStyle(HudButtonStyle::small);

		pos = float2(s_hud_char_icon_size.x + spacing * 2, spacing + topOffset());
		m_name_edit_box = new HudEditBox(FRect(s_name_size) + pos, Character::max_name_size, HudEditBox::mode_locase_nick);
		m_name_edit_box->setLabel("Name: ");

		pos.y += s_name_size.y + spacing;
		m_race_button = new HudClickButton(FRect(s_name_size) + pos);
		m_race_button->setLabelStyle(HudLabelStyle::left);

		pos.y += s_name_size.y * 3 + spacing;
		pos.x = rect().width() - spacing - s_button_size.x;
		m_cancel_button = new HudClickButton(FRect(s_button_size) + pos);
		m_cancel_button->setLabel("cancel");
		m_cancel_button->setButtonStyle(HudButtonStyle::small);
		pos.x -= s_button_size.x + spacing;

		m_create_button = new HudClickButton(FRect(s_button_size) + pos);
		m_create_button->setLabel("create");
		m_create_button->setButtonStyle(HudButtonStyle::small);

		m_icons = game::Character::findIcons();
		ASSERT(!m_icons.empty());

		for(auto &icon : m_icons)
			m_races.emplace_back(icon.first);
		std::sort(m_races.begin(), m_races.end());
		m_races.resize(unique(m_races.begin(), m_races.end()) - m_races.begin());

		m_race_id = 0;
		m_icon_id = -1;

		attach(m_icon_box.get());
		attach(m_name_edit_box.get());
		attach(m_race_button.get());
		attach(m_icon_prev.get());
		attach(m_icon_next.get());
		attach(m_cancel_button.get());
		attach(m_create_button.get());

		updateIcon(0);
	}
		
	HudCharacter::~HudCharacter() { }
		
	bool HudCharacter::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::button_clicked) {
			if(event.source == m_icon_prev)
				updateIcon(-1);
			if(event.source == m_icon_next)
				updateIcon(1);
			if(event.source == m_race_button) {
				m_race_id = (m_race_id + 1) % (int)m_races.size();
				m_icon_id = -1;
				updateIcon(0);
			}

			return true;
		}

		return false;
	}
		
	void HudCharacter::updateIcon(int offset) {
		ProtoIndex index = m_races[m_race_id];

		if(m_icon_id < 0 || m_icon_id >= (int)m_icons.size() || m_icons[m_icon_id].first != index)
			m_icon_id = -1;
		if(m_icon_id == -1) {
			for(int n = 0; n < (int)m_icons.size(); n++)
				if(m_icons[n].first == index)
					m_icon_id = n;
		}

		if(offset) {
			int old_id = m_icon_id;
			m_icon_id += offset;

			while(m_icon_id != old_id) {
				if(m_icon_id == -1)
					m_icon_id = (int)m_icons.size() - 1;
				else if(m_icon_id == (int)m_icons.size())
					m_icon_id = 0;
				else {
					if(m_icons[m_icon_id].first == index)
						break;
					m_icon_id += offset;
				}
			}
		}
		
		const ActorProto &proto = dynamic_cast<const ActorProto&>(getProto(index));
		PCharacter character = new Character("unnamed", m_icon_id == -1? "" : m_icons[m_icon_id].second, proto.id);
		m_icon_box->setCharacter(character);
		m_race_button->setLabel(format("Race: %s", proto.description.c_str()));
	}

	void HudCharacter::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);
	}

}
