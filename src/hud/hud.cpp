/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/hud.h"
#include "hud/main_panel.h"
#include "hud/inventory.h"
#include "hud/options.h"
#include "hud/class.h"
#include "hud/character.h"

#include "game/actor.h"
#include "game/world.h"

using namespace gfx;

namespace hud {

	namespace 
	{

		const float2 s_hud_main_panel_size(365, 160);
		const float2 s_hud_inventory_size(365, 300);
		const float2 s_hud_class_size(365, 300);
		const float2 s_hud_character_size(365, 300);
		const float2 s_hud_options_size(365, 200);

	}

	Hud::Hud(PWorld world) :HudWidget(FRect::empty()), m_world(world), m_selected_layer(layer_none) {
		FRect main_rect = FRect(s_hud_main_panel_size) + float2(layer_spacing, gfx::getWindowSize().y - s_hud_main_panel_size.y - layer_spacing);
		m_main_panel = new HudMainPanel(main_rect);

		FRect inv_rect = align(FRect(s_hud_inventory_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		FRect cls_rect = align(FRect(s_hud_class_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		FRect opt_rect = align(FRect(s_hud_options_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		FRect cha_rect = align(FRect(s_hud_character_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		
		m_layers[layer_inventory]	= new HudInventory(m_world, inv_rect);
		m_layers[layer_class]		= new HudClass(m_world, cls_rect);
		m_layers[layer_character]	= new HudCharacter(cha_rect);
		m_layers[layer_options]		= new HudOptions(opt_rect);

		attach(m_main_panel.get());
		for(auto layer: m_layers) {
			attach(layer.get());
			layer->setVisible(false, false);
		}
	}

	Hud::~Hud() { }
		
	void Hud::setActor(game::EntityRef actor_ref) {
		if(actor_ref == m_actor_ref)
			return;
		m_actor_ref = actor_ref;

	//	m_hud_inventory->setActor(m_actor_ref);
//		if(!m_actor_ref)
//			m_hud_inventory->setVisible(false);
	}
		
	void Hud::setCharacter(game::PCharacter character) {
//		m_character = character;
//		m_hud_char_icon->setCharacter(m_character);
	}
		
	bool Hud::onInput(const io::InputEvent &event) {
		return false;
	}

	bool Hud::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::layer_selected) {
			bool disable = event.value == m_selected_layer;
			m_selected_layer = disable? layer_none : event.value;
			return true;
		}
		else if(event.type == HudEvent::stance_changed) {
			sendOrder(new ChangeStanceOrder((Stance::Type)event.value));
		}

		return false;
	}

	void Hud::onUpdate(double time_diff) {
		bool any_other_visible = false;
		for(int l = 0; l < layer_count; l++)
			if(m_layers[l]->isVisible() && m_selected_layer != l)
				any_other_visible = true;

		float2 main_panel_pos = m_main_panel->rect().min;

		for(int l = 0; l < layer_count; l++) {
			auto &layer = m_layers[l];
			layer->setVisible(isVisible() && m_selected_layer == l && !any_other_visible);
			layer->fitRectToChildren(float2(s_hud_main_panel_size), true);
			FRect layer_rect = layer->targetRect();
			layer->setPos(float2(layer_rect.min.x, main_panel_pos.y - layer_rect.height() - layer_spacing));
		}

		/*{
			float inv_height = m_hud_inventory->preferredHeight();
			FRect inv_rect = m_hud_inventory->targetRect();
			inv_rect.min.y = max(5.0f, inv_rect.max.y - inv_height);
			m_hud_inventory->setTargetRect(inv_rect);
		}*/
	}
		
	void Hud::setVisible(bool is_visible, bool animate) {
		if(!is_visible) {
			m_selected_layer = layer_none;
			for(int l = 0; l < layer_count; l++)
				m_layers[l]->setVisible(false, animate);
		}
		m_main_panel->setVisible(is_visible, animate);
	}
		
	void Hud::sendOrder(POrder &&order) {
		m_world->sendOrder(std::move(order), m_actor_ref);
	}

}
