// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "hud/options.h"
#include "hud/button.h"

namespace hud {

	namespace {

		const float2 s_button_size(240, 24);

	}

	HudOptions::HudOptions(const FRect &target_rect)
		:HudLayer(target_rect) {
		setTitle("Options:");
		
		float2 pos = float2((rect().width() - s_button_size.x) * 0.5f , spacing + topOffset());
		m_exit_to_menu = make_shared<HudClickButton>(FRect(s_button_size) + pos);
		m_exit_to_menu->setLabel("Exit to menu");

		pos += float2(0.0f, s_button_size.y + spacing);
		m_exit_to_system = make_shared<HudClickButton>(FRect(s_button_size) + pos);
		m_exit_to_system->setLabel("Exit to system");

		attach(m_exit_to_menu);
		attach(m_exit_to_system);
	}
		
	
	bool HudOptions::onEvent(const HudEvent &event) {
		if(event.type == HudEvent::button_clicked) {
			if(event.source == m_exit_to_menu)
				handleEvent(HudEvent::exit, 0);
			else if(event.source == m_exit_to_system)
				handleEvent(HudEvent::exit, 1);
		}
		return false;
	}
		
}
