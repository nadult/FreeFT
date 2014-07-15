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

	DECLARE_ENUM(HudButtonStyle,
		normal,
		small
	);

	class HudButton: public HudWidget {
	public:
		HudButton(const FRect &target_rect, int id = 0);
		~HudButton();

		void setButtonStyle(HudButtonStyle::Type style) { m_button_style = style; }

		void setClickSound(HudSound::Type sound) { m_click_sound = sound; }
		void setId(int id) { m_id = id; }
		int id() const { return m_id; }
		
		virtual Color enabledColor() const;
		virtual Color enabledShadowColor() const;
		virtual Color backgroundColor() const;

		float alpha() const { return m_visible_time; }

		void setEnabled(bool is_enabled, bool animate = true);
		bool isEnabled() const { return m_is_enabled; }
	
		bool isHighlighted() const { return m_is_highlighted; }

		void setText(const string &text) { m_text = text; }
		void setIcon(HudIcon::Type icon) { m_icon_id = icon; }
		void setAccelerator(int accel) { m_accelerator = accel; }

	protected:
		void onUpdate(double time_diff) override;
		void onDraw() const override;
		bool onInput(const io::InputEvent&) override;
		void onClick();

		gfx::PTexture m_icons_tex;
		HudIcon::Type m_icon_id;

		string m_text;
		float m_enabled_time;
		float m_highlighted_time;
		HudButtonStyle::Type m_button_style;
		HudSound::Type m_click_sound;
		int m_id, m_group_id, m_accelerator;

		bool m_is_enabled;
		bool m_is_highlighted;
	};

	class HudClickButton: public HudButton {
	public:
		HudClickButton(const FRect &target_rect, int id = 0);
	
	protected:
		bool onInput(const io::InputEvent&) override;

		bool m_is_accelerator;
	};

	class HudToggleButton: public HudButton {
	public:
		HudToggleButton(const FRect &target_rect, int id = 0);
	
	protected:
		bool onInput(const io::InputEvent&) override;
	};

	class HudRadioButton: public HudButton {
	public:
		HudRadioButton(const FRect &target_rect, int id = 0, int group_id = 0);

	protected:
		bool onInput(const io::InputEvent&) override;

		int m_group_id;
	};


}

#endif
