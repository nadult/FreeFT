/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/hud.h"
#include "hud/char_icon.h"
#include "hud/weapon.h"
#include "hud/stance.h"
#include "hud/inventory.h"
#include "hud/options.h"

#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "audio/device.h"

using namespace gfx;

namespace hud {

	namespace 
	{

		const float2 s_hud_char_icon_size(75, 100);
		const float2 s_hud_weapon_size(210, 100);
		const float2 s_hud_button_size(60, 15);
		const float2 s_hud_stance_size(23, 23);
		const float2 s_hud_main_size(365, 155);
		const float2 s_hud_inventory_size(365, 300);
		const float2 s_hud_options_size(365, 200);

		const float s_spacing = 15.0f;
		const float s_layer_spacing = 5.0f;

		enum ButtonId {
			button_inventory,
			button_character,
			button_options,
		};

		struct Button {
			Hud::LayerId id;
			int accelerator;
			const char *name;
		};

		static Button s_buttons[] = {
			{ Hud::layer_inventory,	'I',	"INV" },
			{ Hud::layer_character,	'C',	"CHA" },
			{ Hud::layer_options,	'O',	"OPT" }
		};

	}

	static FRect initRect() {
		return FRect(s_hud_main_size) + float2(s_layer_spacing, gfx::getWindowSize().y - s_hud_main_size.y - s_layer_spacing);
	}

	Hud::Hud(PWorld world, EntityRef actor_ref) :HudLayer(initRect()), m_world(world), m_actor_ref(actor_ref), m_selected_layer(layer_none) {
		float2 bottom_left(s_spacing - s_layer_spacing, rect().height() - s_spacing + s_layer_spacing);

		FRect char_rect(s_hud_char_icon_size);
		char_rect += bottom_left - float2(0, char_rect.height());

		FRect weapon_rect(s_hud_weapon_size);
		weapon_rect += float2(char_rect.max.x + s_spacing, bottom_left.y - weapon_rect.height());

		m_hud_char_icon = new HudCharIcon(char_rect);
		m_hud_weapon = new HudWeapon(weapon_rect);
		m_hud_weapon->setAccelerator('R');

		m_icons = new DTexture; {
			Loader ldr("data/icons.png");
			m_icons->load(ldr);
		}

		{
			FRect stance_rect(s_hud_stance_size);
			stance_rect += float2(weapon_rect.max.x + s_spacing, bottom_left.y - s_hud_stance_size.y);

			FRect uv_rect(0, 0, 0.25f, 0.25f);

			for(int n = 0; n < Stance::count; n++) {
				PHudStance stance(new HudStance(stance_rect, (Stance::Type)n, m_icons));
				m_hud_stances.push_back(std::move(stance));

				uv_rect += float2(0.25f, 0.0f);
				stance_rect += float2(0.0f, -s_hud_stance_size.y - s_spacing);
			}
		}

		{
			FRect button_rect = align(FRect(s_hud_button_size), weapon_rect, align_top, char_rect, align_right, s_spacing);

			for(int n = 0; n < COUNTOF(s_buttons); n++) {
				PHudWidget button(new HudWidget(button_rect));
				button->setText(s_buttons[n].name);
				button->setAccelerator(s_buttons[n].accelerator);
				m_hud_buttons.emplace_back(std::move(button));
				button_rect += float2(button_rect.width() + s_spacing, 0.0f);
			}
		}

		attach(m_hud_weapon.get());
		attach(m_hud_char_icon.get());
		for(int n = 0; n < (int)m_hud_buttons.size(); n++)
			attach(m_hud_buttons[n].get());
		for(int n = 0; n < (int)m_hud_stances.size(); n++)
			attach(m_hud_stances[n].get());

		FRect inv_rect = align(FRect(s_hud_inventory_size) + float2(s_layer_spacing, 0.0f), rect(), align_top, s_layer_spacing);
		m_hud_inventory = new HudInventory(m_world, m_actor_ref, inv_rect);
		m_hud_inventory->setVisible(false, false);
		
		FRect opt_rect = align(FRect(s_hud_options_size) + float2(s_layer_spacing, 0.0f), rect(), align_top, s_layer_spacing);
		m_hud_options = new HudOptions(opt_rect);
		m_hud_options->setVisible(false, false);
	}

	Hud::~Hud() { }

	void Hud::update(bool is_active, double time_diff) {
		float2 mouse_pos = float2(getMousePos()) - rect().min;

		if( const Actor *actor = m_world->refEntity<Actor>(m_actor_ref) ) {
			m_hud_char_icon->setCharacter(actor->character());
			m_hud_char_icon->setHP(actor->hitPoints(), actor->proto().actor->hit_points);

			m_hud_weapon->setWeapon(actor->inventory().weapon());
			m_hud_weapon->setAmmoCount(actor->inventory().ammo().count);

			int stance_id = -1, sel_id = -1;
			bool is_accel = false;

			if(is_active) for(int n = 0; n < (int)m_hud_stances.size(); n++) {
				if(m_hud_stances[n]->isPressed(mouse_pos, 0, &is_accel))
					stance_id = n;
				if(m_hud_stances[n]->isFocused())
					sel_id = n;
			}

			if(stance_id != -1 && stance_id != sel_id) {
				if(!is_accel)
					audio::playSound("butn_pulldown", 1.0f);
				sendOrder(new ChangeStanceOrder(m_hud_stances[stance_id]->stance()));
				for(int n = 0; n < (int)m_hud_stances.size(); n++)
					m_hud_stances[n]->setFocus(stance_id == n);
			}

			if(stance_id == -1 && sel_id == -1)
				for(int n = 0; n < (int)m_hud_stances.size(); n++)
					m_hud_stances[n]->setFocus(m_hud_stances[n]->stance() == actor->stance());

			// Reloading:	
			if(is_active && m_hud_weapon->isPressed(mouse_pos)) {
				const ActorInventory &inventory = actor->inventory();
				const Weapon &weapon = inventory.weapon();
				if(weapon.needAmmo() && inventory.ammo().count < weapon.maxAmmo()) {
					Item ammo = inventory.ammo().item;

					int item_id = -1;

					for(int n = 0; n < inventory.size(); n++)
						if(inventory[n].item == ammo ||
							(ammo.isDummy() && inventory[n].item.type() == ItemType::ammo && Ammo(inventory[n].item).classId() == weapon.ammoClassId())) {
							item_id = n;
							break;
						}

					if(item_id != -1)
						m_world->sendOrder(new EquipItemOrder(item_id), m_actor_ref);
				}
			}
		}

		{
			int pressed_id = -1;

			if(is_active) for(int n = 0; n < (int)m_hud_buttons.size(); n++)
				if(m_hud_buttons[n]->isPressed(mouse_pos))
					pressed_id = n;

			if(pressed_id != -1) {
				bool is_disabling = pressed_id != -1 && m_selected_layer == s_buttons[pressed_id].id;
				audio::playSound("butn_pulldown", 1.0f);
				for(int n = 0; n < (int)m_hud_buttons.size(); n++)
					m_hud_buttons[n]->setFocus(pressed_id == n && !is_disabling);
			
				m_selected_layer = is_disabling? layer_none : s_buttons[pressed_id].id;
			}

			bool any_other_visible = false;

			any_other_visible |= m_selected_layer != layer_inventory && m_hud_inventory->isVisible();
			any_other_visible |= m_selected_layer != layer_options   && m_hud_options->isVisible();

			//TODO: add sliding sounds?
			m_hud_inventory->setVisible(m_selected_layer == layer_inventory && !any_other_visible);
			m_hud_options->setVisible(m_selected_layer == layer_options && !any_other_visible);
		}

		HudLayer::update(is_active, time_diff);
		m_hud_inventory->update(is_active, time_diff);
		m_hud_options->update(is_active, time_diff);

		{
			float inv_height = m_hud_inventory->preferredHeight();
			FRect inv_rect = m_hud_inventory->targetRect();
			inv_rect.min.y = inv_rect.max.y - inv_height;
			m_hud_inventory->setTargetRect(inv_rect);
		}
	}
		
	void Hud::setVisible(bool is_visible, bool animate) {
		HudLayer::setVisible(is_visible, animate);
		if(!is_visible) {
			m_hud_options->setVisible(false, animate);
			m_hud_inventory->setVisible(false, animate);
			for(int n = 0; n < (int)m_hud_buttons.size(); n++)
				m_hud_buttons[n]->setFocus(false);
			m_selected_layer = layer_none;
		}
	}
		
	void Hud::sendOrder(POrder &&order) {
		m_world->sendOrder(std::move(order), m_actor_ref);
	}

	void Hud::draw() const {
		HudLayer::draw();
		m_hud_inventory->draw();
		m_hud_options->draw();
	}

	bool Hud::isMouseOver() const {
		return HudLayer::isMouseOver() || m_hud_inventory->isMouseOver() || m_hud_options->isMouseOver();
	}

}
