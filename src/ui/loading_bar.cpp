// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "ui/loading_bar.h"

#include "res_manager.h"
#include <fwk/gfx/canvas_2d.h>
#include <fwk/gfx/font.h>
#include <fwk/vulkan/vulkan_image.h>

namespace ui {

LoadingBar::LoadingBar()
	: m_font(&res::getFont("transformers_30")), m_tex(res::getTexture("loading_bar.png", true)) {
	// TODO
	//m_tex->setFiltering(TextureFilterOpt::linear);
}
FWK_COPYABLE_CLASS_IMPL(LoadingBar)

void LoadingBar::draw(Canvas2D &out, int2 pos, Maybe<float> progress) const {
	FColor color(1.0f, 0.8f, 0.2f, m_alpha);
	out.pushViewMatrix();
	out.setViewPos(-pos);

	int2 dims(m_tex->size2D());
	float2 center = float2(dims.x * 0.49f, dims.y * 0.49f);

	float scale = 1.0f + pow(sin(m_anim_pos * 0.5 * pi * 2.0), 8.0) * 0.1;

	const char *text = "Loading";
	FontStyle style{IColor(color), ColorId::black, HAlign::right, VAlign::center};
	FRect extents = m_font->draw(out, float2(), style, text);

	out.mulViewMatrix(translation(extents.ex() + 8.0f + center.x, 0.0f, 0.0f));
	out.mulViewMatrix(scaling(scale));
	out.mulViewMatrix(rotation(float3(0, 0, 1), m_anim_pos * 2.0f * pi));
	out.mulViewMatrix(translation(-center.x, -center.y, 0.0f));

	out.setMaterial({m_tex, ColorId::white, SimpleBlendingMode::normal});
	out.addFilledRect(FRect(IRect(dims)), color);
	out.setMaterial({});
	out.popViewMatrix();
}

void LoadingBar::animate(double time_diff, float alpha) {
	m_anim_pos += time_diff;
	if(m_anim_pos > 1.0)
		m_anim_pos -= 1.0;
	m_alpha = alpha;
}

}
