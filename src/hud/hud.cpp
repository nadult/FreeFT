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
#include "game/game_mode.h"

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

	Hud::Hud(PWorld world) :HudWidget(FRect::empty()), m_selected_layer(layer_none) {
		DASSERT(world);

		FRect main_rect = FRect(s_hud_main_panel_size) + float2(layer_spacing, gfx::getWindowSize().y - s_hud_main_panel_size.y - layer_spacing);
		m_main_panel = new HudMainPanel(world, main_rect);

		FRect inv_rect = align(FRect(s_hud_inventory_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		FRect cls_rect = align(FRect(s_hud_class_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		FRect opt_rect = align(FRect(s_hud_options_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		FRect cha_rect = align(FRect(s_hud_character_size) + float2(layer_spacing, 0.0f), main_rect, align_top, layer_spacing);
		
		m_layers[layer_inventory]	= new HudInventory(world, inv_rect);
		m_layers[layer_class]		= new HudClass(world, cls_rect);
		m_layers[layer_character]	= new HudCharacter(world, cha_rect);
		m_layers[layer_options]		= new HudOptions(opt_rect);

		attach(m_main_panel.get());
		for(auto layer: m_layers) {
			attach(layer.get());
			layer->setVisible(false, false);
		}
	}

	Hud::~Hud() { }
		
	bool Hud::onInput(const io::InputEvent &event) {
		return false;
	}

	bool Hud::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::layer_selected) {
			bool disable = event.value == m_selected_layer;
			m_selected_layer = disable? layer_none : event.value;
			if(m_selected_layer != layer_none)
				setVisible(true, true);
			return true;
		}

		return false;
	}

	void Hud::onUpdate(double time_diff) {
		bool any_other_visible = false;
		for(int l = 0; l < layer_count; l++) {
			if(m_layers[l]->isVisible() && m_selected_layer != l)
				any_other_visible = true;
		}

		//TODO: canShow for layers
		float2 main_panel_pos = m_main_panel->rect().min;

		for(int l = 0; l < layer_count; l++) {
			auto &layer = m_layers[l];
			layer->setVisible(isVisible() && m_selected_layer == l && !any_other_visible);
			layer->fitRectToChildren(float2(s_hud_main_panel_size), true);
			FRect layer_rect = layer->targetRect();
			layer->setPos(float2(layer_rect.min.x, main_panel_pos.y - layer_rect.height() - layer_spacing));
		}
	}
		
	void Hud::setVisible(bool is_visible, bool animate) {
		if(!is_visible) {
			m_selected_layer = layer_none;
			for(int l = 0; l < layer_count; l++)
				m_layers[l]->setVisible(false, animate);
		}
		m_main_panel->setVisible(is_visible, animate);
	}

	bool Hud::isVisible() const {
		return m_main_panel->isVisible();
	}
		
	void Hud::setPC(game::PPlayableCharacter pc) {
		m_main_panel->setPC(pc);
		for(auto &layer: m_layers)
			layer->setPC(pc);
	}

}
