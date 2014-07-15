/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CLASS_H
#define HUD_CLASS_H

#include "hud/layer.h"
#include "hud/button.h"

namespace hud
{

	class HudClassButton: public HudRadioButton {
	public:
		enum { spacing = 17 };

		HudClassButton(const FRect &rect);

		void onDraw() const override;
		Color backgroundColor() const override;
	};

	class HudClass: public HudLayer {
	public:
		HudClass(const FRect &target_rect);
		~HudClass();

		void onUpdate(double time_diff) override;
		int selectedId() const { return m_selected_id; }
		void select(int id) { m_selected_id = id; }

	private:
		int m_offset, m_class_count;
		int m_selected_id;

		vector<Ptr<HudClassButton>> m_buttons;
		PHudButton m_button_up, m_button_down;
	};

}

#endif
