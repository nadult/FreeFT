/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/hud.h"
#include "hud/button.h"
#include "hud/char_icon.h"
#include "hud/weapon.h"
#include "hud/stance.h"

#include "game/actor.h"
#include "game/world.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "audio/device.h"

using namespace gfx;

namespace 
{

	const int2 s_hud_char_icon_size(75, 100);
	const int2 s_hud_weapon_size(210, 100);
	const int2 s_hud_button_size(60, 15);
	const int2 s_hud_stance_size(23, 23);
	const int2 s_hud_main_rect(365, 155);
	const float s_spacing = 15.0f;

}

namespace hud {

	Hud::Hud(PWorld world, EntityRef actor_ref) :m_world(world), m_actor_ref(actor_ref) {
		HudStyle style = getStyle((HudStyleId::Type)0);

		int2 res = gfx::getWindowSize();
		float bottom = res.y - s_spacing;

		FRect char_rect({0.0f, 0.0f}, s_hud_char_icon_size);
		char_rect += float2(s_spacing, bottom - char_rect.height());

		FRect weapon_rect({0.0f, 0.0f}, s_hud_weapon_size);
		weapon_rect += float2(char_rect.max.x + s_spacing, bottom - weapon_rect.height());

		m_hud_char_icon.reset(new HudCharIcon(char_rect));
		m_hud_weapon.reset(new HudWeapon(weapon_rect));

		m_icons = new DTexture; {
			Loader ldr("data/icons.png");
			m_icons->load(ldr);
		}

		{
			FRect stance_rect({0.0f, 0.0f}, s_hud_stance_size);
			stance_rect += float2(weapon_rect.max.x + s_spacing, bottom - s_hud_stance_size.y);

			FRect uv_rect(0, 0, 0.25f, 0.25f);

			for(int n = 0; n < Stance::count; n++) {
				PHudStance stance(new HudStance(stance_rect, (Stance::Type)n, m_icons));
				m_hud_stances.push_back(std::move(stance));

				uv_rect += float2(0.25f, 0.0f);
				stance_rect += float2(0.0f, -s_hud_stance_size.y - s_spacing);
			}
		}

		{
			FRect button_rect({0.0f, 0.0f}, s_hud_button_size);

			button_rect += float2(char_rect.max.x + s_spacing, bottom - weapon_rect.height() - button_rect.height() - s_spacing);
			PHudButton inv_button(new HudButton(button_rect));

			button_rect += float2(button_rect.width() + s_spacing, 0.0f);
			PHudButton cha_button(new HudButton(button_rect));
		
			button_rect += float2(button_rect.width() + s_spacing, 0.0f);
			PHudButton opt_button(new HudButton(button_rect));

			inv_button->setText("INV");
			cha_button->setText("CHA");
			opt_button->setText("OPT");

			m_hud_buttons.emplace_back(std::move(inv_button));
			m_hud_buttons.emplace_back(std::move(cha_button));
			m_hud_buttons.emplace_back(std::move(opt_button));
		}

		m_all_buttons.push_back(m_hud_weapon.get());
		m_all_buttons.push_back(m_hud_char_icon.get());
		for(int n = 0; n < (int)m_hud_buttons.size(); n++)
			m_all_buttons.push_back(m_hud_buttons[n].get());
		for(int n = 0; n < (int)m_hud_stances.size(); n++)
			m_all_buttons.push_back(m_hud_stances[n].get());

		m_back_rect = FRect({0.0f, 0.0f}, s_hud_main_rect);
		m_back_rect += float2(5.0f, res.y - m_back_rect.height() - 5.0f);

		setStyle(defaultStyle());
	}

	void Hud::setStyle(HudStyle style) {
		m_style = style;
		for(int n = 0; n < (int)m_all_buttons.size(); n++)
			m_all_buttons[n]->setStyle(style);
	}

	Hud::~Hud() { }

	void Hud::update(bool is_active, double time_diff) {
		const Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		if(!actor)
			return;

		m_hud_char_icon->setCharacter(actor->character());
		m_hud_char_icon->setHP(actor->hitPoints(), actor->proto().actor->hit_points);

		m_hud_weapon->setWeapon(actor->inventory().weapon());
		m_hud_weapon->setAmmoCount(actor->inventory().ammo().count);
		

		{
			int stance_id = -1, sel_id = -1;

			if(is_active) for(int n = 0; n < (int)m_hud_stances.size(); n++) {
				if( ( m_hud_stances[n]->isMouseOver() && isMouseKeyDown(0) ) || m_hud_stances[n]->testAccelerator())
					stance_id = n;
				if(m_hud_stances[n]->isFocused())
					sel_id = n;
			}

			if(stance_id != -1 && stance_id != sel_id) {
				audio::playSound("butn_pulldown", 1.0f);
				sendOrder(new ChangeStanceOrder(m_hud_stances[stance_id]->stance()));
				for(int n = 0; n < (int)m_hud_stances.size(); n++)
					m_hud_stances[n]->setFocus(stance_id == n);
			}

			if(stance_id == -1 && sel_id == -1)
				for(int n = 0; n < (int)m_hud_stances.size(); n++)
					m_hud_stances[n]->setFocus(m_hud_stances[n]->stance() == actor->stance());
		}

		for(int n = 0; n < (int)m_all_buttons.size(); n++)
			m_all_buttons[n]->update(time_diff);
	}
		
	void Hud::sendOrder(POrder &&order) {
		m_world->sendOrder(std::move(order), m_actor_ref);
	}

	void Hud::draw() const {
		drawLayer(m_back_rect, m_style.layer_color);

		for(int n = 0; n < (int)m_all_buttons.size(); n++)
			m_all_buttons[n]->draw();
	}

	bool Hud::isMouseOver() const {
		for(int n = 0; n < (int)m_all_buttons.size(); n++)
			if(m_all_buttons[n]->isMouseOver())
				return true;
		return m_back_rect.isInside(getMousePos());
	}

}
