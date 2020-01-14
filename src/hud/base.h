// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/base.h"
#include <fwk/gfx/renderer2d.h>
#include <fwk/gfx/gl_texture.h>

namespace hud {

	using namespace game;
	
	void drawGradQuad(Renderer2D&, const FRect &rect, Color a, Color b, bool is_vertical);
	void drawLine(Renderer2D&, float2 p1, float2 p2, Color a, Color b);
	void drawBorder(Renderer2D&, const FRect &rect, Color color, const float2 &offset, float width);
	void animateValue(float &value, float speed, bool maximize);

	DEFINE_ENUM(HudSound,
		none,
		button,
		item_equip,
		error
	);

	enum HudLayerId {
		layer_none = -1,

		layer_inventory,
		layer_character,
		layer_options,
		layer_class,
		layer_stats,

		layer_count
	};

	class HudWidget;

	struct HudEvent {
		enum Type {
			button_clicked,
			text_modified,
			row_clicked,

			layer_changed,
			item_equip,
			item_unequip,
			item_drop,
			item_focused,
			exit
		};

		HudEvent(HudWidget *source, Type type, int value = 0)
			:source(source), type(type), value(value) { }

		HudWidget *source;
		Type type;
		int value;
	};


	void playSound(HudSound);

	enum Alignment {
		align_top,
		align_right,
		align_left,
		align_down,

		align_top_left,
		align_top_right,
		align_down_left,
		align_down_right,
	};

	const FRect align(const FRect &rect, const FRect &relative_to, Alignment mode, float spacing);
	inline const FRect align(const FRect &rect, const FRect &rel1, Alignment mode1, const FRect &rel2, Alignment mode2, float spacing)
		{ return align(align(rect, rel1, mode1, spacing), rel2, mode2, spacing); }

	struct HudStyle {
		Color layer_color;
		Color back_color;
		Color border_color;
		Color enabled_color;
		const char *font_name;
		const char *big_font_name;
	};

	DEFINE_ENUM(HudStyleId,
		green_white,
		red_green,
		console
	);

	HudStyle getStyle(HudStyleId);
	inline HudStyle defaultStyle() { return getStyle(HudStyleId::green_white); }

	class Hud;
	class HudLayer;
	class HudWidget;
	class HudButton;
	class HudWeapon;
	class HudStance;
	class HudCharIcon;
	class HudItemButton;
	class HudItemDesc;
	class HudEditBox;
	class HudGrid;
	class HudTargetInfo;

	class HudMainPanel;
	class HudInventory;
	class HudCharacter;
	class HudClass;
	class HudStats;
	class HudOptions;

	class SinglePlayerMenu;
	class MultiPlayerMenu;
	class ServerMenu;

	using PHudWidget = shared_ptr<HudWidget>;
	using PHudButton = shared_ptr<HudButton>;
	using PHudEditBox = shared_ptr<HudEditBox>;
	using PHudLayer = shared_ptr<HudLayer>;
	using PHudWeapon = shared_ptr<HudWeapon>;
	using PHudStance = shared_ptr<HudStance>;
	using PHudCharIcon = shared_ptr<HudCharIcon>;
	using PHudItemButton = shared_ptr<HudItemButton>;
	using PHudItemDesc = shared_ptr<HudItemDesc>;
	using PHudGrid = shared_ptr<HudGrid>;
	using PHud = shared_ptr<Hud>;
	using PHudMainPanel = shared_ptr<HudMainPanel>;
	
	using PMultiPlayerMenu = shared_ptr<MultiPlayerMenu>;
	using PSinglePlayerMenu = shared_ptr<SinglePlayerMenu>;
	using PServerMenu = shared_ptr<ServerMenu>;

}
