/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_INVENTORY_H
#define HUD_INVENTORY_H

#include "hud/layer.h"
#include "hud/button.h"
#include "game/item.h"
#include "game/inventory.h"

namespace hud
{

	class HudItemDesc: public HudButton {
	public:
		HudItemDesc(const FRect &rect);
		void setItem(const Item &item) { m_item = item; }
		void onDraw() const override;

	protected:
		Item m_item;
	};

	class HudInventoryItem: public HudButton {
	public:
		HudInventoryItem(const FRect &rect);
		void setItem(const Item &item) { m_item = item; }
		void setCount(int count) { m_count = count; }

		const Item &item() const { return m_item; }
		int count() const { return m_count; }

		void onDraw() const override;
		
		Color backgroundColor() const override;

	protected:
		Item m_item;
		int m_count;
	};

	class HudInventory: public HudLayer {
	public:
		enum { spacing = 17 };

		HudInventory(const FRect &target_rect);
		~HudInventory();

		float preferredHeight() const;
		void setActor(game::EntityRef);
		bool canShow() const override;

	protected:
		bool onInput(const io::InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onUpdate(double time_diff) override;
		void onDraw() const override;

	private:
		int m_row_offset;
		int m_min_items;

		vector<PHudInventoryItem> m_buttons;
		PHudButton m_button_up, m_button_down;
		PHudItemDesc m_item_desc;
		float m_out_of_item_time;

		float2 m_drop_start_pos;
		game::Item m_drop_item;
		double m_drop_count;
	};

}

#endif
