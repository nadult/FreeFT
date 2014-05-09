/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "gameui/inventory_menu.h"
#include "gfx/device.h"

namespace ui
{

	InventoryMenu::InventoryMenu(const IRect &screen_rect) :Window(IRect(0, 0, 10, 10)) {
		gfx::PTexture back = gfx::DTexture::gui_mgr["back/lequip"];
		setBackground(back);
		int2 pos = screen_rect.center() - back->dimensions() / 2;
		setRect(IRect(pos, pos + back->dimensions()));
	}

}
