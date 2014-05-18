/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_INVENTORY_H
#define HUD_INVENTORY_H

#include "hud/layer.h"
#include "hud/widget.h"
#include "game/item.h"
#include "game/inventory.h"

namespace hud
{

	class HudItemDesc: public HudWidget {
	public:
		HudItemDesc(const FRect &rect);
		void setItem(const Item &item) { m_item = item; }
		void draw() const override;

	protected:
		Item m_item;
	};

	class HudInventoryItem: public HudWidget {
	public:
		HudInventoryItem(const FRect &rect);
		void setItem(const Item &item) { m_item = item; }
		void setCount(int count) { m_count = count; }

		const Item &item() const { return m_item; }
		int count() const { return m_count; }

		void draw() const override;
		
		Color backgroundColor() const override;

	protected:
		Item m_item;
		int m_count;
	};

	class HudInventory: public HudLayer {
	public:
		enum { spacing = 17 };

		HudInventory(PWorld world, EntityRef actor_ref, const FRect &target_rect);
		~HudInventory();

		float preferredHeight() const;
		void update(bool is_active, double time_diff) override;
		void draw() const override;

	private:
		game::PWorld m_world;
		game::EntityRef m_actor_ref;

		vector<PHudInventoryItem> m_buttons;
		PHudItemDesc m_item_desc;
		float m_out_of_item_time;

		float2 m_drop_start_pos;
		game::Item m_drop_item;
		double m_drop_count;
	};

}

#endif
