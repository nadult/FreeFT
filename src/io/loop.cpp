// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "io/loop.h"

#include "gfx/drawing.h"

#include <fwk/gfx/canvas_2d.h>
#include <fwk/sys/input.h>

namespace io {

Loop::Loop() : m_is_transitioning(false), m_is_exiting(false) {}

Loop::~Loop() {}

bool Loop::tick(double time_diff) {
	if(m_is_transitioning) {
		m_transition.pos += time_diff;
		if(m_transition.pos > m_transition.length) {
			m_is_transitioning = false;
			onTransitionFinished();
		}
	}

	return !m_is_exiting && onTick(time_diff);
}

void Loop::exit() { m_is_exiting = true; }

void Loop::startTransition(Color from, Color to, TransitionMode mode, float length) {
	m_transition = Transition{FColor(from), FColor(to), mode, 0.0f, length};
	m_is_transitioning = true;
}

bool Loop::isTransitioning() const { return m_is_transitioning; }

void Loop::Transition::draw(Canvas2D &canvas, const IRect &rect) const {
	canvas.setMaterial({ColorId::white, SimpleBlendingMode::normal});
	if(mode == trans_normal) {
		canvas.addFilledRect(rect, lerp(from, to, pos / length));
	} else {
		FColor col1 = from, col2 = to;
		if(mode == trans_right)
			swap(col1, col2);

		FRect rects[3] = {FRect(rect), FRect(rect), FRect(rect)};
		float anim_pos = pos / length;
		for(int n = 0; n < arraySize(rects); n++)
			rects[n] += float2(rect.width() * (mode == trans_right ? n - 2 + 2.0f * anim_pos :
																	 n - 2.0f * anim_pos),
							   0.0f);

		FColor mid_colors[4] = {col1, col1, col2, col2};
		FRect uv_rect(0, 0, 1, 1);

		canvas.addFilledRect(rects[0], col1);
		canvas.addFilledRect(rects[1], uv_rect, mid_colors);
		canvas.addFilledRect(rects[2], col2);
	}
	canvas.setMaterial({});
}

void Loop::draw(Canvas2D &canvas) {
	onDraw(canvas);
	if(m_is_transitioning)
		m_transition.draw(canvas, canvas.viewport());
}

}
