// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "hud/button.h"
#include "hud/layer.h"

namespace hud {

class HudClassButton : public HudRadioButton {
  public:
	static constexpr int spacing = 17;

	HudClassButton(const FRect &rect);

	void onDraw(Renderer2D &) const override;
	Color backgroundColor() const override;
};

using PHudClassButton = shared_ptr<HudClassButton>;

class HudClass : public HudLayer {
  public:
	HudClass(const FRect &target_rect);
	~HudClass();

	bool canShow() const override;

  private:
	void onUpdate(double time_diff) override;
	bool onEvent(const HudEvent &) override;
	void onLayout() override;
	void onPCControllerSet() override;

	vector<int> m_class_ids;
	vector<PHudClassButton> m_buttons;
	PHudButton m_button_up, m_button_down;
	int m_offset;
};

}
