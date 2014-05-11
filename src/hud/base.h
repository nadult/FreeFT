/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_BASE_H
#define HUD_BASE_H

#include "game/base.h"

namespace hud {

	using namespace game;
	
	void drawGradQuad(const FRect &rect, Color a, Color b, bool is_vertical);
	void drawLine(float2 p1, float2 p2, Color a, Color b);
	void drawBorder(const FRect &rect, Color color, const float2 &offset, float width, bool is_left);
	void drawLayer(const FRect &rect, Color color);

	struct HudStyle {
		Color layer_color;
		Color back_color;
		Color border_color;
		Color focus_color;
		float border_offset;
		const char *font_name;
	};

	DECLARE_ENUM(HudStyleId,
		green_blue,
		red_green
	);

	HudStyle getStyle(HudStyleId::Type);
	inline HudStyle defaultStyle() { return getStyle(HudStyleId::green_blue); }

	class Hud;
	class HudButton;
	class HudWeapon;
	class HudStance;
	class HudCharIcon;
	class HudInventory;
	class HudCharacter;
	class HudOptions;

	typedef unique_ptr<HudButton> PHudButton;
	typedef unique_ptr<HudWeapon> PHudWeapon;
	typedef unique_ptr<HudStance> PHudStance;
	typedef unique_ptr<HudCharIcon> PHudCharIcon;
	typedef unique_ptr<HudInventory> PHudInventory;
	typedef unique_ptr<HudCharacter> PHudCharacter;
	typedef unique_ptr<HudOptions> PHudOptions;
	typedef Ptr<Hud> PHud;

}

#endif
