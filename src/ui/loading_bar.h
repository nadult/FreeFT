// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "base.h"
#include <fwk/vulkan_base.h>

namespace ui {

class LoadingBar {
  public:
	LoadingBar();
	FWK_COPYABLE_CLASS(LoadingBar);

	void draw(Canvas2D &out, int2 pos, Maybe<float> progress = none) const;
	void animate(double time_diff, float alpha = 1.0f);

  private:
	const Font *m_font;
	PVImageView m_tex;
	double m_anim_pos = 0.0f;
	float m_alpha = 1.0f;
};

}
