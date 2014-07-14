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
			int accelerator;
			const char *name;
		};

		static Button s_buttons[] = {
			{ layer_inventory,		'I',	"INV" },
			{ layer_character,		'C',	"CHA" },
			{ layer_options,		'O',	"OPT" },
			{ layer_class,			'R',	"CLS" }
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
				PHudButton stance(new HudButton(stance_rect, s_stance_buttons[n].stance_id));
				stance->setIcon(s_stance_buttons[n].icon_id);
				m_hud_stances.push_back(std::move(stance));

				stance_rect += float2(0.0f, -s_hud_stance_size.y - spacing);
			}
		}

		{
			FRect button_rect = align(FRect(s_hud_button_size), char_rect, align_top, spacing);
			button_rect += float2(char_rect.min.x - button_rect.min.x, 0.0f);

			for(int n = 0; n < COUNTOF(s_buttons); n++) {
				PHudButton button(new HudButton(button_rect, s_buttons[n].layer_id));
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
		
/*	void HudMainPanel::setActor(game::EntityRef actor_ref) {
		if(actor_ref == m_actor_ref)
			return;
		m_actor_ref = actor_ref;

		m_hud_inventory->setActor(m_actor_ref);
		if(!m_actor_ref)
			m_hud_inventory->setVisible(false);
	}
		
	void HudMainPanel::setCharacter(game::PCharacter character) {
		m_character = character;
		m_hud_char_icon->setCharacter(m_character);
	}*/
		
	bool HudMainPanel::onInput(const io::InputEvent &event) {
		return false;
	}
	
	template <class T>
	static bool isOneOf(HudWidget *source, const vector<Ptr<T>> &widgets) {
		for(auto &widget : widgets)
			if(widget.get() == source)
				return true;
		return false;
	}

	bool HudMainPanel::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::button_clicked) {
		   	HudButton *source = dynamic_cast<HudButton*>(event.source);
			if(isOneOf(source, m_hud_buttons)) {
				playSound(HudSound::button);
				bool disable_all = source->isEnabled();
				handleEvent(HudEvent::layer_changed, disable_all? layer_none : source->id());
			}
			else if(isOneOf(source, m_hud_stances) && !source->isEnabled() && m_pc_controller->canChangeStance()) {
				playSound(HudSound::button);
				m_pc_controller->setStance((Stance::Type)source->id());
			}
				
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

		int stance = m_pc_controller->targetStance();
		for(auto &button: m_hud_stances)
			button->setEnabled(button->id() == stance);

	/*	if( const Actor *actor = m_world->refEntity<Actor>(m_actor_ref) ) {
			m_hud_char_icon->setHP(actor->hitPoints(), actor->proto().actor->hit_points);

			m_hud_weapon->setWeapon(actor->inventory().weapon());
			m_hud_weapon->setAmmoCount(actor->inventory().ammo().count);

			int stance_id = -1, sel_id = -1;
			bool is_accel = false;

			for(int n = 0; n < (int)m_hud_stances.size(); n++) {
				if(m_hud_stances[n]->isPressed(mouse_pos, 0, &is_accel))
					stance_id = n;
				if(m_hud_stances[n]->isFocused())
					sel_id = n;
			}

			if(handle_input || (is_accel && handle_accelerators)) {
				if(stance_id != -1 && stance_id != sel_id) {
					playSound(HudSound::button);

					for(int n = 0; n < (int)m_hud_stances.size(); n++)
						m_hud_stances[n]->setFocus(stance_id == n);
				}

				if(stance_id == -1 && sel_id == -1)
					for(int n = 0; n < (int)m_hud_stances.size(); n++)
						m_hud_stances[n]->setFocus(s_stance_buttons[n].stance_id == actor->stance());
			}

			bool is_pressed = m_hud_weapon->isPressed(mouse_pos, 0, &is_accel);
			if(is_pressed && (handle_input || (is_accel && handle_accelerators))) {
				const ActorInventory &inventory = actor->inventory();
				const Weapon &weapon = inventory.weapon();
				if(weapon.needAmmo() && inventory.ammo().count < weapon.maxAmmo()) {
					int item_id = inventory.find(inventory.ammo().item);
					if(item_id == -1) for(int n = 0; n < inventory.size(); n++)
						if(weapon.canUseAmmo(inventory[n].item)) {
							item_id = n;
							break;
						}
					if(item_id != -1)
						m_world->sendOrder(new EquipItemOrder(inventory[item_id].item), m_actor_ref);
				}
			}
		}

		{
			int pressed_id = -1;

			if(handle_input) for(int n = 0; n < (int)m_hud_buttons.size(); n++)
				if(m_hud_buttons[n]->isPressed(mouse_pos))
					pressed_id = n;

			if(pressed_id != -1) {
				bool is_disabling = pressed_id != -1 && m_selected_layer == s_buttons[pressed_id].id;
				playSound(HudSound::button);
				for(int n = 0; n < (int)m_hud_buttons.size(); n++)
					m_hud_buttons[n]->setFocus(pressed_id == n && !is_disabling);
			
				m_selected_layer = is_disabling? layer_none : s_buttons[pressed_id].id;
				if(m_selected_layer == layer_inventory && !m_actor_ref)
					m_selected_layer = layer_none;
			}

		}*/
	}

}
