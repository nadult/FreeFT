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
	void drawBorder(const FRect &rect, Color color, const float2 &offset, float width);
	void animateValue(float &value, float speed, bool maximize);

	enum class HudSound {
		button,
		item_equip,
		error,

		count
	};

	enum HudLayerId {
		layer_none = -1,

		layer_inventory,
		layer_character,
		layer_options,
		layer_class,

		layer_count
	};

	class HudWidget;

	struct HudEvent {
		enum Type {
			button_clicked,
			text_modified,

			layer_selected,
			stance_changed,

			item_equipped,
			item_unequipped,
			item_dropped,
		};

		HudEvent(HudWidget *source, Type type, int value = 0) :source(source), type(type), value(value) { }

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

	DECLARE_ENUM(HudStyleId,
		green_white,
		red_green
	);

	HudStyle getStyle(HudStyleId::Type);
	inline HudStyle defaultStyle() { return getStyle(HudStyleId::green_white); }

	class Hud;
	class HudLayer;
	class HudWidget;
	class HudButton;
	class HudWeapon;
	class HudStance;
	class HudCharIcon;
	class HudInventoryItem;
	class HudItemDesc;

	class HudMainPanel;
	class HudInventory;
	class HudCharacter;
	class HudOptions;
	class HudClass;

	class SinglePlayerMenu;
	class MultiPlayerMenu;
	class ServerMenu;

	typedef Ptr<HudWidget> PHudWidget;
	typedef Ptr<HudButton> PHudButton;
	typedef Ptr<HudLayer> PHudLayer;
	typedef Ptr<HudWeapon> PHudWeapon;
	typedef Ptr<HudStance> PHudStance;
	typedef Ptr<HudCharIcon> PHudCharIcon;
	typedef Ptr<HudInventoryItem> PHudInventoryItem;
	typedef Ptr<HudItemDesc> PHudItemDesc;
	typedef Ptr<Hud> PHud;
	
	typedef Ptr<MultiPlayerMenu> PMultiPlayerMenu;
	typedef Ptr<SinglePlayerMenu> PSinglePlayerMenu;
	typedef Ptr<ServerMenu> PServerMenu;

}

#endif
