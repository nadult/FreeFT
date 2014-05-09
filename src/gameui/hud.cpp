/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gameui/hud.h"
#include "game/actor.h"
#include "gfx/device.h"

using namespace game;

namespace ui {

	HUD::HUD() :Window(IRect({0, 0}, gfx::getWindowSize())) {
		IRect screen_rect({0, 0}, gfx::getWindowSize());

		m_bottom_menu = new BottomMenu(screen_rect);
		m_inventory_menu = new InventoryMenu(screen_rect);
		attach(m_bottom_menu.get());
		attach(m_inventory_menu.get());

		m_inventory_menu->setVisible(false);
	}

	void HUD::update(Actor &actor) {
		Stance::Type stance = actor.stance();
		m_bottom_menu->setStance(stance);
		m_bottom_menu->setSlotItem(actor.inventory().weapon(), 0);
	}

	bool HUD::isMouseOver() const {
		int2 mouse_pos = gfx::getMousePos();
		if(m_bottom_menu->rect().isInside(mouse_pos))
			return true;
		if(m_inventory_menu->isVisible() && m_inventory_menu->rect().isInside(mouse_pos))
			return true;

		return false;
	}

}
