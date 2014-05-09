/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAMEUI_BOTTOM_MENU_H
#define GAMEUI_BOTTOM_MENU_H

#include "ui/window.h"
#include "ui/image_button.h"
#include "game/base.h"
#include "game/item.h"

namespace ui
{

	class SlotButton: public ImageButton {
	public:
		SlotButton(const int2 &pos, int slot_id);
		void drawContents() const override;

		void setItem(const game::Item &item) {
			m_item = item;
		}

	private:
		game::Item m_item;
	};

	typedef Ptr<SlotButton> PSlotButton;

	class BottomMenu: public Window {
	public:
		enum {
			max_slots = 2
		};

		enum SubMenuId {
			sub_menu_inventory,
			sub_menu_character,
			sub_menu_options,
			sub_menu_exit,
			sub_menu_dummy,

			sub_menu_count
		};

		BottomMenu(const IRect &screen_rect);
		~BottomMenu();
		
		void drawContents() const override;
	
		void setStance(game::Stance::Type);
		game::Stance::Type stance() const;

		void setSentryMode(game::SentryMode::Type);
		game::SentryMode::Type sentryMode() const;

		void setSlotItem(game::Item, int slot_id = 0);

		bool onEvent(const Event &ev) override;

	private:
		PImageButton m_stance_buttons[game::Stance::count];
		PImageButton m_sentry_buttons[game::SentryMode::count];
		PImageButton m_sub_menu_buttons[sub_menu_count];
		PSlotButton  m_slot_buttons[max_slots];

		vector<gfx::PTexture> m_minimap_noise;
		mutable int m_noise_id;
		mutable double m_last_time;
	};

	typedef Ptr<BottomMenu> PBottomMenu;

}

#endif
