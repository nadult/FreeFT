/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAMEUI_INVENTORY_MENU_H
#define GAMEUI_INVENTORY_MENU_H

#include "ui/window.h"

namespace ui
{

	class InventoryMenu: public Window {
	public:
		InventoryMenu(const IRect &screen_rect);
	};

	typedef Ptr<InventoryMenu> PInventoryMenu;

}

#endif
