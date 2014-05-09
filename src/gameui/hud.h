/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAMEUI_HUD_H
#define GAMEUI_HUD_H

#include "ui/window.h"

namespace ui
{

	class HUD: public Window {
	public:
		HUD();
		void drawContents() const override;

	private:
		gfx::PTexture m_back;
	};

	typedef Ptr<HUD> PHUD;

}

#endif
