/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/hud.h"
#include "hud/char_icon.h"
#include "hud/weapon.h"
#include "hud/inventory.h"
#include "hud/options.h"
#include "hud/class.h"

#include "game/actor.h"
#include "game/world.h"
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
		const float2 s_hud_main_size(365, 155);
		const float2 s_hud_inventory_size(365, 300);
		const float2 s_hud_class_size(365, 300);
		const float2 s_hud_options_size(365, 200);

		enum ButtonId {
			button_inventory,
			button_character,
			button_options,
			button_class,
		};

		struct Button {
			Hud::LayerId id;
			int accelerator;
			const char *name;
		};

		static Button s_buttons[] = {
			{ Hud::layer_inventory,		'I',	"INV" },
			{ Hud::layer_character,		'C',	"CHA" },
			{ Hud::layer_options,		'O',	"OPT" },
			{ Hud::layer_class,			'R',	"CLS" }
		};

		struct StanceButton {
			Stance::Type stance_id;
			HudIcon::Type icon_id;
			int accelerator;
		};

		static StanceButton s_stance_buttons[] = {
			{ Stance::prone,  HudIcon::stance_prone,  'Z' },
			{ Stance::crouch, HudIcon::stance_crouch, 'A' },
			{ Stance::stand,  HudIcon::stance_stand,  'Q' },
		};

	}

	static FRect initRect() {
		return FRect(s_hud_main_size) + float2(HudLayer::spacing, gfx::getWindowSize().y - s_hud_main_size.y - HudLayer::spacing);
	}

	Hud::Hud(PWorld world) :HudLayer(initRect()), m_world(world), m_selected_layer(layer_none) {
		float2 bottom_left(HudWidget::spacing - spacing, rect().height() - HudWidget::spacing + HudLayer::spacing);

		FRect char_rect(s_hud_char_icon_size);
		char_rect += bottom_left - float2(0, char_rect.height());

		FRect weapon_rect(s_hud_weapon_size);
		weapon_rect += float2(char_rect.max.x + HudWidget::spacing, bottom_left.y - weapon_rect.height());

		m_hud_char_icon = new HudCharIcon(char_rect);
		m_hud_weapon = new HudWeapon(weapon_rect);
		m_hud_weapon->setAccelerator('R');

		{
			FRect stance_rect(s_hud_stance_size);
			stance_rect += float2(weapon_rect.max.x + HudWidget::spacing, bottom_left.y - s_hud_stance_size.y);

			for(int n = 0; n < COUNTOF(s_stance_buttons); n++) {
				PHudWidget stance(new HudWidget(stance_rect));
				stance->setIcon(s_stance_buttons[n].icon_id);
				stance->setAccelerator(s_stance_buttons[n].accelerator);
				m_hud_stances.push_back(std::move(stance));

				stance_rect += float2(0.0f, -s_hud_stance_size.y - HudWidget::spacing);
			}
		}

		{
			FRect button_rect = align(FRect(s_hud_button_size), char_rect, align_top, HudWidget::spacing);
			button_rect += float2(char_rect.min.x - button_rect.min.x, 0.0f);

			for(int n = 0; n < COUNTOF(s_buttons); n++) {
				PHudWidget button(new HudWidget(button_rect));
				button->setText(s_buttons[n].name);
				button->setAccelerator(s_buttons[n].accelerator);
				m_hud_buttons.emplace_back(std::move(button));
				button_rect += float2(button_rect.width() + HudWidget::spacing, 0.0f);
			}
		}

		attach(m_hud_weapon.get());
		attach(m_hud_char_icon.get());
		for(int n = 0; n < (int)m_hud_buttons.size(); n++)
			attach(m_hud_buttons[n].get());
		for(int n = 0; n < (int)m_hud_stances.size(); n++)
			attach(m_hud_stances[n].get());

		FRect inv_rect = align(FRect(s_hud_inventory_size) + float2(spacing, 0.0f), rect(), align_top, spacing);
		m_hud_inventory = new HudInventory(m_world, inv_rect);
		m_hud_inventory->setVisible(false, false);
		
		FRect cls_rect = align(FRect(s_hud_class_size) + float2(spacing, 0.0f), rect(), align_top, spacing);
		m_hud_class = new HudClass(m_world, cls_rect);
		m_hud_class->setVisible(false, false);
		
		FRect opt_rect = align(FRect(s_hud_options_size) + float2(spacing, 0.0f), rect(), align_top, spacing);
		m_hud_options = new HudOptions(opt_rect);
		m_hud_options->setVisible(false, false);
	}

	Hud::~Hud() { }
		
	void Hud::setActor(game::EntityRef actor_ref) {
		if(actor_ref == m_actor_ref)
			return;
		m_actor_ref = actor_ref;

		m_hud_inventory->setActor(m_actor_ref);
		if(!m_actor_ref)
			m_hud_inventory->setVisible(false);
	}
		
	void Hud::setCharacter(game::PCharacter character) {
		m_character = character;
		m_hud_char_icon->setCharacter(m_character);
	}

	void Hud::update(bool is_active, double time_diff) {
		float2 mouse_pos = float2(getMousePos()) - rect().min;
		is_active &= isVisible() && m_visible_time == 1.0f;

		if( const Actor *actor = m_world->refEntity<Actor>(m_actor_ref) ) {
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

			if(stance_id != -1 && stance_id != sel_id) {
				playSound(HudSound::button);

				sendOrder(new ChangeStanceOrder(s_stance_buttons[stance_id].stance_id));
				for(int n = 0; n < (int)m_hud_stances.size(); n++)
					m_hud_stances[n]->setFocus(stance_id == n);
			}

			if(stance_id == -1 && sel_id == -1)
				for(int n = 0; n < (int)m_hud_stances.size(); n++)
					m_hud_stances[n]->setFocus(s_stance_buttons[n].stance_id == actor->stance());

			// Reloading:	
			if(m_hud_weapon->isPressed(mouse_pos)) {
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

			if(is_active) for(int n = 0; n < (int)m_hud_buttons.size(); n++)
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

			bool any_other_visible = false;

			any_other_visible |= m_selected_layer != layer_inventory && m_hud_inventory->isVisible();
			any_other_visible |= m_selected_layer != layer_options   && m_hud_options->isVisible();
			any_other_visible |= m_selected_layer != layer_class     && m_hud_class->isVisible();

			//TODO: add sliding sounds?
			m_hud_inventory->setVisible(isVisible() && m_selected_layer == layer_inventory && !any_other_visible);
			m_hud_options->setVisible(isVisible() && m_selected_layer == layer_options && !any_other_visible);
			m_hud_class->setVisible(isVisible() && m_selected_layer == layer_class && !any_other_visible);
		}

		HudLayer::update(is_active, time_diff);
		m_hud_inventory->update(is_active, time_diff);
		m_hud_options->update(is_active, time_diff);
		m_hud_class->update(is_active, time_diff);

		{
			float inv_height = m_hud_inventory->preferredHeight();
			FRect inv_rect = m_hud_inventory->targetRect();
			inv_rect.min.y = max(5.0f, inv_rect.max.y - inv_height);
			m_hud_inventory->setTargetRect(inv_rect);
		}
		{
			float cls_height = m_hud_class->preferredHeight();
			FRect cls_rect = m_hud_class->targetRect();
			cls_rect.min.y = max(5.0f, cls_rect.max.y - cls_height);
			m_hud_class->setTargetRect(cls_rect);
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
		m_hud_class->draw();
	}

	bool Hud::isMouseOver() const {
		return HudLayer::isMouseOver() || m_hud_inventory->isMouseOver() || m_hud_options->isMouseOver() || m_hud_class->isMouseOver();
	}

}
