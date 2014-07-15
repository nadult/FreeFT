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
#include "gfx/device.h"
#include "gfx/font.h"

using namespace gfx;

namespace hud {

	namespace 
	{

		const float2 s_hud_char_icon_size(75, 100);
		const float2 s_hud_weapon_size(210, 100);
		const float2 s_hud_button_size(60, 17);
		const float2 s_hud_stance_size(23, 23);

		struct Button {
			int layer_id;
			const char *name;
		};

		static Button s_buttons[] = {
			{ layer_inventory,		"INV" },
			{ layer_character,		"CHAR" },
			{ layer_options,		"OPT" },
			{ layer_class,			"CLASS" }
		};

		struct StanceButton {
			Stance::Type stance_id;
			HudIcon::Type icon_id;
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
		weapon_rect += float2(char_rect.max.x + spacing, bottom_left.y - weapon_rect.height());

		m_hud_char_icon = new HudCharIcon(char_rect);
		m_hud_weapon = new HudWeapon(weapon_rect);

		{
			FRect stance_rect(s_hud_stance_size);
			stance_rect += float2(weapon_rect.max.x + spacing, bottom_left.y - s_hud_stance_size.y);

			for(int n = 0; n < COUNTOF(s_stance_buttons); n++) {
				PHudButton stance(new HudRadioButton(stance_rect, s_stance_buttons[n].stance_id, 1));
				stance->setIcon(s_stance_buttons[n].icon_id);
				m_hud_stances.push_back(std::move(stance));

				stance_rect += float2(0.0f, -s_hud_stance_size.y - spacing);
			}
		}

		{
			FRect button_rect = align(FRect(s_hud_button_size), char_rect, align_top, spacing);
			button_rect += float2(char_rect.min.x - button_rect.min.x, 0.0f);

			for(int n = 0; n < COUNTOF(s_buttons); n++) {
				PHudButton button(new HudToggleButton(button_rect, s_buttons[n].layer_id));
				button->setText(s_buttons[n].name);
				m_hud_buttons.emplace_back(std::move(button));
				button_rect += float2(button_rect.width() + spacing, 0.0f);
			}
		}

		attach(m_hud_weapon.get());
		attach(m_hud_char_icon.get());
		for(int n = 0; n < (int)m_hud_buttons.size(); n++)
			attach(m_hud_buttons[n].get());
		for(int n = 0; n < (int)m_hud_stances.size(); n++)
			attach(m_hud_stances[n].get());
	}

	HudMainPanel::~HudMainPanel() { }
		
	bool HudMainPanel::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::button_clicked) {
		   	HudButton *source = dynamic_cast<HudButton*>(event.source);
			if(isOneOf(source, m_hud_buttons)) {
				handleEvent(HudEvent::layer_changed, source->id());
			}
			else if(isOneOf(source, m_hud_stances) && m_pc_controller->canChangeStance()) {
				m_pc_controller->setStance((Stance::Type)event.value);
			}
			else if(m_hud_weapon == source)
				m_pc_controller->reload();
				
			return true;
		}

		return false;
	}
		
	void HudMainPanel::setLayerId(int layer_id) {
		for(auto &button: m_hud_buttons)
			button->setEnabled(button->id() == layer_id);
	}

	void HudMainPanel::onUpdate(double time_diff) {
		HudLayer::onUpdate(time_diff);

		ASSERT(m_pc_controller); //TODO

		int stance = m_pc_controller->targetStance();
		for(auto &button: m_hud_stances)
			button->setEnabled(button->id() == stance);

		const Actor *actor = m_pc_controller->actor();

		if( actor)  {
			m_hud_char_icon->setHP(actor->hitPoints(), actor->proto().actor->hit_points);
			m_hud_char_icon->setCharacter(new Character(m_pc_controller->pc().character()));

			m_hud_weapon->setWeapon(actor->inventory().weapon());
			m_hud_weapon->setAmmoCount(actor->inventory().ammo().count);
		}
	}

}
