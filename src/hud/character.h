/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_CHARACTER_H
#define HUD_CHARACTER_H

#include "hud/layer.h"
#include "hud/button.h"
#include "hud/char_icon.h"
#include "hud/edit_box.h"
#include "game/character.h"

namespace hud
{

	class HudCharacter: public HudLayer {
	public:
		enum { spacing = 17 };

		HudCharacter(const FRect &target_rect);
		~HudCharacter();

	private:
		void onUpdate(double time_diff) override;
		bool onEvent(const HudEvent&) override;
		void onPCControllerSet() override;
		void onLayout() override;

		void updateIconId(int offset);
		void updateClassId();

		PPlayableCharacter makePC();

		int m_icon_id, m_race_id, m_class_id;
		vector<pair<ProtoIndex, string>> m_icons;
		vector<int> m_class_ids;
		vector<ProtoIndex> m_races;

		PHudCharIcon m_icon_box;
		PHudEditBox m_name_edit_box;
		PHudButton m_race_button, m_class_button;
		PHudButton m_icon_next, m_icon_prev;
		PHudButton m_create_button, m_cancel_button;
	};

}

#endif
