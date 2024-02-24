// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/hud.h"
#include "hud/character.h"
#include "hud/class.h"
#include "hud/inventory.h"
#include "hud/main_panel.h"
#include "hud/options.h"
#include "hud/stats.h"

#include "game/actor.h"
#include "game/game_mode.h"
#include "game/pc_controller.h"
#include "game/world.h"

namespace hud {

namespace {

	const float2 s_hud_main_panel_size(365, 160);
	const float2 s_hud_inventory_size(365, 300);
	const float2 s_hud_class_size(365, 300);
	const float2 s_hud_character_size(365, 300);
	const float2 s_hud_options_size(365, 200);
	const float2 s_hud_stats_size(365, 200);

}

Hud::Hud(PWorld world, const int2 &window_size)
	: HudWidget({}), m_selected_layer(layer_none), m_exit_value(0) {
	DASSERT(world);

	FRect main_rect =
		FRect(s_hud_main_panel_size) +
		float2(layer_spacing, window_size.y - s_hud_main_panel_size.y - layer_spacing);
	m_main_panel = make_shared<HudMainPanel>(main_rect);

	FRect inv_rect = align(FRect(s_hud_inventory_size) + float2(layer_spacing, 0.0f), main_rect,
						   align_top, layer_spacing);
	FRect cls_rect = align(FRect(s_hud_class_size) + float2(layer_spacing, 0.0f), main_rect,
						   align_top, layer_spacing);
	FRect opt_rect = align(FRect(s_hud_options_size) + float2(layer_spacing, 0.0f), main_rect,
						   align_top, layer_spacing);
	FRect cha_rect = align(FRect(s_hud_character_size) + float2(layer_spacing, 0.0f), main_rect,
						   align_top, layer_spacing);
	FRect sta_rect = align(FRect(s_hud_stats_size) + float2(layer_spacing, 0.0f), main_rect,
						   align_top, layer_spacing);

	m_layers[layer_inventory] = make_shared<HudInventory>(inv_rect);
	m_layers[layer_class] = make_shared<HudClass>(cls_rect);
	m_layers[layer_character] = make_shared<HudCharacter>(cha_rect);
	m_layers[layer_stats] = make_shared<HudStats>(sta_rect);
	m_layers[layer_options] = make_shared<HudOptions>(opt_rect);

	attach(m_main_panel);
	for(auto layer : m_layers) {
		attach(layer);
		layer->setVisible(false, false);
	}

	setWorld(world);
	setPCController(PPCController());
}

Hud::~Hud() {}

bool Hud::onInput(const InputEvent &event) { return false; }

bool Hud::onEvent(const HudEvent &event) {
	if(event.type == HudEvent::layer_changed) {
		DASSERT(event.value >= layer_none && event.value < layer_count);
		m_selected_layer = event.value == m_selected_layer ? layer_none : event.value;
		if(m_selected_layer != layer_none && !m_layers[m_selected_layer]->canShow())
			m_selected_layer = layer_none;

		if(m_selected_layer != layer_none)
			setVisible(true, true);
		return true;
	} else if(event.type == HudEvent::exit) {
		m_exit_value = 1 + event.value;
	}

	return false;
}

void Hud::showLayer(int layer_id) { handleEvent(HudEvent::layer_changed, layer_id); }

void Hud::onLayout() {
	//TODO: canShow for layers
	float2 main_panel_pos = m_main_panel->rect().min();

	for(auto &layer : m_layers) {
		layer->fitRectToChildren(float2(s_hud_main_panel_size), true);
		FRect layer_rect = layer->targetRect();
		layer->setPos(
			float2(layer_rect.x(), main_panel_pos.y - layer_rect.height() - layer_spacing));
	}
}

void Hud::onUpdate(double time_diff) {
	bool any_other_visible = false;
	for(int l = 0; l < layer_count; l++) {
		if(m_layers[l]->isVisible() && m_selected_layer != l)
			any_other_visible = true;
	}
	if(m_selected_layer != layer_none && !m_layers[m_selected_layer]->canShow())
		m_selected_layer = layer_none;

	for(int l = 0; l < layer_count; l++) {
		m_main_panel->setCanShowLayer(l, m_layers[l]->canShow());
		m_layers[l]->setVisible(isVisible() && m_selected_layer == l && !any_other_visible);
	}

	m_main_panel->setCurrentLayer(m_selected_layer);
}

void Hud::setVisible(bool is_visible, bool animate) {
	if(!is_visible) {
		m_selected_layer = layer_none;
		for(int l = 0; l < layer_count; l++)
			m_layers[l]->setVisible(false, animate);
	}
	m_main_panel->setVisible(is_visible, animate);
}

bool Hud::isVisible() const { return m_main_panel->isVisible(); }

void Hud::setWorld(game::PWorld world) {
	m_main_panel->setWorld(world);
	for(auto &layer : m_layers)
		layer->setWorld(world);
}

void Hud::setPCController(game::PPCController pc_controller) {
	m_main_panel->setPCController(pc_controller);
	for(auto &layer : m_layers)
		layer->setPCController(pc_controller);
}

}
