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

	struct HudItemEntry {
		HudItemEntry() :count(0), is_equipped(false) { }
		HudItemEntry(Item item, int count, bool is_equipped)
			:item(item), count(count), is_equipped(is_equipped) { }

		Item item;
		int count;
		bool is_equipped;

		bool operator<(const HudItemEntry&) const;
		bool operator==(const HudItemEntry&) const;
	};

	class HudItemDesc: public HudButton {
	public:
		HudItemDesc(const FRect &rect);
		void setItem(const Item &item) { m_item = item; }

	protected:
		void onDraw() const override;

		Item m_item;
	};

	class HudItemButton: public HudButton {
	public:
		HudItemButton(const FRect &rect);

		void setEntry(const HudItemEntry&);
		const HudItemEntry &entry() const { return m_entry; }

		Color backgroundColor() const override;

		bool isDropping() const { return m_drop_count >= 0.0; }
		int dropCount() const { return (int)m_drop_count; }

	protected:
		bool onInput(const io::InputEvent&) override;
		void onUpdate(double time_diff) override;
		void onDraw() const override;

		HudItemEntry m_entry;
		double m_drop_count;
		float  m_drop_diff;
		float2 m_drop_start_pos;
	};

	class HudInventory: public HudLayer {
	public:
		enum { spacing = 17 };

		HudInventory(const FRect &target_rect);
		~HudInventory();

		void updateData();

		bool canShow() const override;

	protected:
		bool onInput(const io::InputEvent&) override;
		bool onEvent(const HudEvent&) override;
		void onUpdate(double time_diff) override;
		void onLayout() override;
		void onDraw() const override;

	private:
		int m_row_offset, m_max_row_offset;

		vector<HudItemEntry> m_entries;
		vector<PHudItemButton> m_buttons;
		PHudButton m_button_up, m_button_down;
		PHudItemDesc m_item_desc;
		float m_out_of_item_time;
	};

}

#endif
