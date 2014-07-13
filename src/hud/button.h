/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_BUTTON_H
#define HUD_BUTTON_H

#include "hud/widget.h"

namespace hud
{

	DECLARE_ENUM(HudIcon,
		undefined = -1,
		stance_stand,
		stance_crouch,
		stance_prone,

		down_arrow,
		up_arrow,
		left_arrow,
		right_arrow,

		close
	);

	class HudButton: public HudWidget {
	public:
		enum { spacing = 15 };

		HudButton(const FRect &target_rect, HudEvent::Type type = HudEvent::button_clicked, int event_value = 0);
		virtual ~HudButton();

		void onUpdate(double time_diff) override;
		void onDraw() const override;
		bool onInput(const io::InputEvent&) override;
		
		virtual Color enabledColor() const;
		virtual Color enabledShadowColor() const;
		virtual Color backgroundColor() const;

		float alpha() const { return m_visible_time; }

		void setEnabled(bool is_enabled, bool animate = true);
		bool isEnabled() const { return m_is_enabled; }
	
		bool isHighlighted() const { return m_is_highlighted; }

		void setText(const string &text) { m_text = text; }
		void setIcon(HudIcon::Type icon) { m_icon_id = icon; }

		void setAccelerator(int key) { m_accelerator = key; }
		int accelerator() const { return m_accelerator; }

	protected:
		gfx::PTexture m_icons_tex;
		HudIcon::Type m_icon_id;

		string m_text;
		float m_enabled_time;
		float m_highlighted_time;
		
		const HudEvent::Type m_event_type;
		const int m_event_value;

		int m_accelerator;
		bool m_is_enabled;
		bool m_is_highlighted;

	};

}

#endif
