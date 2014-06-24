/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CLASS_H
#define HUD_CLASS_H

#include "hud/layer.h"
#include "hud/widget.h"

namespace hud
{

	class HudClassButton: public HudWidget {
	public:
		enum { spacing = 17 };

		HudClassButton(const FRect &rect);

		void setId(int predefined_id);

		void draw() const override;
		Color backgroundColor() const override;

	protected:
		int m_predefined_id;
	};

	class HudClass: public HudLayer {
	public:
		HudClass(PWorld world, EntityRef actor_ref, const FRect &target_rect);
		~HudClass();

		float preferredHeight() const;
		void update(bool is_active, double time_diff) override;
		void draw() const override;

		int selectedId() const { return m_selected_id; }
		void select(int id) { m_selected_id = id; }

	private:
		game::PWorld m_world;
		game::EntityRef m_actor_ref;
		int m_offset, m_class_count;
		int m_selected_id;

		vector<Ptr<HudClassButton>> m_buttons;
		PHudWidget m_button_up, m_button_down;
	};

}

#endif
