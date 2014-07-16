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

	DECLARE_ENUM(HudLabelStyle,
		left,
		center,
		right,
		disabled
	);

	class HudButton: public HudWidget {
	public:
		HudButton(const FRect &target_rect, int id = 0);
		~HudButton();

		void setButtonStyle(HudButtonStyle::Type style) { m_button_style = style; }
		void setLabelStyle(HudLabelStyle::Type style) { m_label_style = style; }

		void setClickSound(HudSound::Type sound) { m_click_sound = sound; }
		void setId(int id) { m_id = id; }
		int id() const { return m_id; }
		
		virtual Color textColor(bool force_enabled = false) const;
		virtual Color textShadowColor() const;
		virtual Color backgroundColor() const;
		virtual Color borderColor() const;

		float alpha() const { return m_visible_time; }

		void setEnabled(bool is_enabled, bool animate = true);
		bool isEnabled() const { return m_is_enabled; }

		void setHighlighted(bool is_highlighted, bool animate = true);	
		bool isHighlighted() const { return m_is_highlighted; }

		void setGreyed(bool is_greyed, bool animate = true);
		bool isGreyed() const { return m_is_greyed; }

		void setLabel(const string &label) { m_label = label; }
		const string &label() const { return m_label; }

		void setIcon(HudIcon::Type icon) { m_icon_id = icon; }
		void setAccelerator(int accel) { m_accelerator = accel; }

	protected:
		void onUpdate(double time_diff) override;
		void onDraw() const override;
		bool onInput(const io::InputEvent&) override;
		void onClick();

		gfx::PTexture m_icons_tex;
		HudIcon::Type m_icon_id;

		string m_label;
		float m_enabled_time;
		float m_highlighted_time;
		float m_greyed_time;
		HudButtonStyle::Type m_button_style;
		HudLabelStyle::Type m_label_style;
		HudSound::Type m_click_sound;
		int m_id, m_group_id, m_accelerator;

		bool m_is_greyed;
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
