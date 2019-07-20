// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "hud/widget.h"

namespace hud
{

	DEFINE_ENUM(HudIcon,
		stance_stand,
		stance_crouch,
		stance_prone,

		down_arrow,
		up_arrow,
		left_arrow,
		right_arrow,

		close
	);

	DEFINE_ENUM(HudButtonStyle,
		normal,
		small
	);

	DEFINE_ENUM(HudLabelStyle,
		left,
		center,
		right,
		disabled
	);

	class HudButton: public HudWidget {
	public:
		HudButton(const FRect &target_rect, int id = 0);
		~HudButton();

		void setButtonStyle(HudButtonStyle style) { m_button_style = style; }
		void setLabelStyle(HudLabelStyle style) { m_label_style = style; }

		void setClickSound(HudSound sound) { m_click_sound = sound; }
		void setId(int id) { m_id = id; }
		int id() const { return m_id; }
		
		virtual Color textColor(bool force_enabled = false) const;
		virtual Color textShadowColor() const;
		virtual Color backgroundColor() const;
		virtual Color borderColor() const;

		void setEnabled(bool is_enabled, bool animate = true);
		bool isEnabled() const { return m_is_enabled; }

		void setHighlighted(bool is_highlighted, bool animate = true);	
		bool isHighlighted() const { return m_is_highlighted; }

		void setGreyed(bool is_greyed, bool animate = true);
		bool isGreyed() const { return m_is_greyed; }

		void setLabel(const string &label) { m_label = label; }
		const string &label() const { return m_label; }

		void setIcon(HudIcon icon) { m_icon_id = icon; }
		void setAccelerator(int accel) { m_accelerator = accel; }

	protected:
		void onUpdate(double time_diff) override;
		void onDraw(Renderer2D&) const override;
		bool onInput(const InputEvent&) override;
		void onClick();

		PTexture m_icons_tex;
		Maybe<HudIcon> m_icon_id;

		string m_label;
		float m_enabled_time = 0.0f;
		float m_highlighted_time = 0.0f;
		float m_greyed_time = 0.0f;
		HudButtonStyle m_button_style;
		HudLabelStyle m_label_style;
		HudSound m_click_sound = HudSound::button;
		int m_id, m_group_id, m_accelerator = 0;

		bool m_is_greyed = false;
		bool m_is_enabled = false;
		bool m_is_highlighted = false;
	};

	class HudClickButton: public HudButton {
	public:
		HudClickButton(const FRect &target_rect, int id = 0);
	
	protected:
		bool onInput(const InputEvent&) override;

		bool m_is_accelerator;
	};

	class HudToggleButton: public HudButton {
	public:
		HudToggleButton(const FRect &target_rect, int id = 0);
	
	protected:
		bool onInput(const InputEvent&) override;
	};

	class HudRadioButton: public HudButton {
	public:
		HudRadioButton(const FRect &target_rect, int id = 0, int group_id = 0);

	protected:
		bool onInput(const InputEvent&) override;

		int m_group_id;
	};


}
