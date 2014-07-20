/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/character.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/pc_controller.h"
#include "game/game_mode.h"
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
		:HudLayer(target_rect), m_race_id(0), m_class_id(0), m_icon_id(0) {
		setTitle(" ");

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
		
		pos.y += s_name_size.y + spacing;
		m_class_button = new HudClickButton(FRect(s_name_size) + pos);
		m_class_button->setLabelStyle(HudLabelStyle::left);
		
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

		attach(m_icon_box.get());
		attach(m_name_edit_box.get());
		attach(m_race_button.get());
		attach(m_class_button.get());
		attach(m_icon_prev.get());
		attach(m_icon_next.get());
		attach(m_cancel_button.get());
		attach(m_create_button.get());
	}
		
	HudCharacter::~HudCharacter() { }
		
	bool HudCharacter::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::button_clicked && !m_pc_controller) {
			if(event.source == m_icon_prev)
				updateIconId(-1);
			if(event.source == m_icon_next)
				updateIconId(1);
			if(event.source == m_class_button)
				m_class_id = (m_class_id + 1) % (int)CharacterClass::count();
			if(event.source == m_race_button) {
				m_race_id = (m_race_id + 1) % (int)m_races.size();
				m_icon_id = 0;
			}
			if(event.source == m_create_button && m_world && !m_pc_controller) {
				PPlayableCharacter pc = makePC();
				GameModeClient *game_mode = dynamic_cast<GameModeClient*>(m_world->gameMode());
				if(game_mode && pc)
					game_mode->addPC(*pc);
			}

			needsLayout();

			return true;
		}

		return false;
	}
		
	void HudCharacter::onPCControllerSet() {
		m_icon_next->setGreyed((bool)m_pc_controller);
		m_icon_prev->setGreyed((bool)m_pc_controller);
		m_cancel_button->setGreyed((bool)m_pc_controller);

		if(!m_pc_controller) {
			GameModeClient *game_mode = dynamic_cast<GameModeClient*>(m_world->gameMode());
			if(game_mode)
				m_name_edit_box->setText(game_mode->currentNickName());
			m_race_id = m_class_id = m_icon_id = 0;
		}

		needsLayout();
	}

	void HudCharacter::updateIconId(int offset) {
		if(m_pc_controller)
			return;

		ProtoIndex index = m_race_id == -1? ProtoIndex() : m_races[m_race_id];

		if(m_icons[m_icon_id].first != index)
			for(int n = 0; n < (int)m_icons.size(); n++)
				if(m_icons[n].first == index)
					m_icon_id = n;
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
	}

	void HudCharacter::updateClassId() {
		ProtoIndex index = m_races[m_race_id];
		const ActorProto &proto = dynamic_cast<const ActorProto&>(getProto(index));

		int iters = 0, default_id = CharacterClass::defaultId();
		while((!CharacterClass::get(m_class_id).isValidForActor(proto.id) || m_class_id == default_id) && iters++ < CharacterClass::count())
			m_class_id = (m_class_id + 1) % CharacterClass::count();
		if(iters == CharacterClass::count())
			m_class_id = default_id;
	}

	void HudCharacter::onLayout() {
		HudLayer::onLayout();
		setTitle(m_pc_controller? "Character: " : "Character creation: ");

		if(m_pc_controller) {
			const PlayableCharacter &pc = m_pc_controller->pc();
			const ActorProto &proto = dynamic_cast<const ActorProto&>(pc.character().proto());
			m_icon_box->setCharacter(new Character(pc.character()));
			m_race_button->setLabel(format("Race: %s", proto.description.c_str()));
			m_class_button->setLabel(format("Class: %s", pc.characterClass().name().c_str()));
			m_name_edit_box->setText(pc.character().name());
		}
		else {
			ProtoIndex index = m_races[m_race_id];
			const ActorProto &proto = dynamic_cast<const ActorProto&>(getProto(index));

			updateIconId(0);
			updateClassId();
	
			m_icon_box->setCharacter(new Character("", m_icon_id == -1? "" : m_icons[m_icon_id].second, proto.id));
			m_race_button->setLabel(format("Race: %s", proto.description.c_str()));
			m_class_button->setLabel(format("Class: %s", CharacterClass::get(m_class_id).name().c_str()));
		}

	}

	void HudCharacter::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);
		if(m_pc_controller) {
			m_name_edit_box->setInputFocus(false);
			m_race_button->setEnabled(false);
			m_class_button->setEnabled(false);
		}

		m_create_button->setGreyed((bool)m_pc_controller || m_name_edit_box->text().empty());
	}

	PPlayableCharacter HudCharacter::makePC() {
		if(m_pc_controller || m_name_edit_box->text().empty())
			return PPlayableCharacter();
			
		updateIconId(0);
		updateClassId();
			
		return new PlayableCharacter(
				Character(m_name_edit_box->text(), m_icons[m_icon_id].second, getProto(m_races[m_race_id]).id),
				m_class_id );
	}

}
