/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/main_panel.h"
#include "hud/button.h"
#include "hud/char_icon.h"
#include "hud/weapon.h"

#include "game/actor.h"
#include "game/world.h"
#include "game/game_mode.h"
#include "game/pc_controller.h"

using namespace gfx;

namespace hud {

	namespace 
	{

		const float2 s_hud_char_icon_size(75, 100);
		const float2 s_hud_weapon_size(210, 100);
		const float2 s_hud_button_size(56, 17);
		const float2 s_hud_stance_size(23, 23);

		struct Button {
			int layer_id;
			const char *name;
		};

		static Button s_buttons[] = {
			{ layer_inventory,		"INV" },
			{ layer_character,		"CHAR" },
			{ layer_class,			"CLASS" },
			{ layer_stats,			"STATS" },
			{ layer_options,		"OPT" }
		};

		struct StanceButton {
			Stance stance_id;
			HudIcon icon_id;
		};

		static StanceButton s_stance_buttons[] = {
			{ Stance::prone,  HudIcon::stance_prone },
			{ Stance::crouch, HudIcon::stance_crouch },
			{ Stance::stand,  HudIcon::stance_stand }
		};

	}

	HudMainPanel::HudMainPanel(const FRect &rect) :HudLayer(rect) {
		float2 bottom_left(spacing, rect.height() - spacing);

		FRect char_rect(s_hud_char_icon_size);
		char_rect += bottom_left - float2(0, char_rect.height());

		FRect weapon_rect(s_hud_weapon_size);
		weapon_rect += float2(char_rect.ex() + spacing, bottom_left.y - weapon_rect.height());

		m_hud_char_icon = make_shared<HudCharIcon>(char_rect);
		m_hud_weapon = make_shared<HudWeapon>(weapon_rect);

		{
			FRect stance_rect(s_hud_stance_size);
			stance_rect += float2(weapon_rect.ex() + spacing, bottom_left.y - s_hud_stance_size.y);

			for(int n = 0; n < arraySize(s_stance_buttons); n++) {
				PHudButton stance(new HudRadioButton(stance_rect, (int)s_stance_buttons[n].stance_id, 1));
				stance->setIcon(s_stance_buttons[n].icon_id);
				m_hud_stances.push_back(std::move(stance));

				stance_rect += float2(0.0f, -s_hud_stance_size.y - spacing);
			}
		}

		{
			FRect button_rect = align(FRect(s_hud_button_size), char_rect, align_top, spacing);
			button_rect += float2(char_rect.x() - button_rect.x(), 0.0f);

			for(int n = 0; n < arraySize(s_buttons); n++) {
				PHudButton button(new HudToggleButton(button_rect, s_buttons[n].layer_id));
				button->setLabel(s_buttons[n].name);
				m_hud_buttons.emplace_back(std::move(button));
				button_rect += float2(button_rect.width() + spacing, 0.0f);
			}
		}

		attach(m_hud_weapon);
		attach(m_hud_char_icon);
		for(int n = 0; n < (int)m_hud_buttons.size(); n++)
			attach(m_hud_buttons[n]);
		for(int n = 0; n < (int)m_hud_stances.size(); n++)
			attach(m_hud_stances[n]);
	}

	HudMainPanel::~HudMainPanel() { }
		
	bool HudMainPanel::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::button_clicked) {
		   	HudButton *source = dynamic_cast<HudButton*>(event.source);
			if(isOneOf(source, m_hud_buttons)) {
				handleEvent(HudEvent::layer_changed, source->id());
			}

			if(!m_pc_controller)
				return false;

			if(isOneOf(source, m_hud_stances) && m_pc_controller->canChangeStance()) {
				m_pc_controller->setStance((Stance)event.value);
			}
			if(m_hud_weapon.get() == source)
				m_pc_controller->reload();
				
			return true;
		}

		return false;
	}
		
		
	void HudMainPanel::setCanShowLayer(int layer_id, bool can_show) {
		for(auto &button: m_hud_buttons)
			if(button->id() == layer_id)
				button->setGreyed(!can_show);
	}

	void HudMainPanel::setCurrentLayer(int layer_id) {
		for(auto &button: m_hud_buttons)
			button->setEnabled(button->id() == layer_id);
	}

	void HudMainPanel::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);

		const Character *character = m_pc_controller? &m_pc_controller->pc().character() : nullptr;
		const Actor *actor = m_pc_controller? m_pc_controller->actor() : nullptr;

		int stance = m_pc_controller? m_pc_controller->targetStance() : -1;
		if(actor && actor->isDead())
			stance = (int)Stance::prone;

		for(auto &button: m_hud_stances) {
			button->setEnabled(button->id() == stance);
			button->setGreyed(stance == -1 || !actor || actor->isDead());
		}
		m_hud_weapon->setGreyed(!actor || actor->isDead());
			
		m_hud_char_icon->setCharacter(character? make_shared<Character>(*character) : PCharacter());

		if(actor) {
			m_hud_char_icon->setHP(actor->hitPoints(), actor->proto().actor->hit_points);
			m_hud_weapon->setWeapon(actor->inventory().weapon());
			m_hud_weapon->setAmmoCount(actor->inventory().ammo().count);
		}
		else {
			m_hud_char_icon->setHP(0, 0);
			m_hud_weapon->setWeapon(Item::dummyWeapon());
			m_hud_weapon->setAmmoCount(0);
		}
	}

}
