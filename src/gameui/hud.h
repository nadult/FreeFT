/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAMEUI_HUD_H
#define GAMEUI_HUD_H

#include "ui/window.h"
#include "gameui/bottom_menu.h"
#include "gameui/inventory_menu.h"

namespace ui
{

	class HUD: public Window {
	public:
		HUD();

		void update(game::Actor&);
		bool isMouseOver() const;

	private:
		PBottomMenu m_bottom_menu;
		PInventoryMenu m_inventory_menu;
	};

}

#endif
