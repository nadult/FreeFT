/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/loop.h"
#include "gfx/drawing.h"

namespace io {

	Loop::Loop() :m_is_transitioning(false), m_is_exiting(false) { }

	Loop::~Loop() { }
		
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

	void Loop::exit() {
		m_is_exiting = true;
	}
		
	void Loop::startTransition(Color from, Color to, TransitionMode mode, float length) {
		m_transition = Transition{ from, to, mode, 0.0f, length };
		m_is_transitioning = true;
	}
		
	bool Loop::isTransitioning() const {
		return m_is_transitioning;
	}
			
	void Loop::Transition::draw(const IRect &rect) {
		Renderer2D renderer(rect);

		if(mode == trans_normal) {
			renderer.addFilledRect(rect, lerp(from, to, pos / length));
		}
		else {
			Color col1 = from, col2 = to;
			if(mode == trans_right)
				swap(col1, col2);

			FRect rects[3] = { FRect(rect), FRect(rect), FRect(rect) };
			float anim_pos = pos / length;
			for(int n = 0; n < arraySize(rects); n++)
				rects[n] += float2(rect.width() * (mode == trans_right? n - 2 + 2.0f * anim_pos : n - 2.0f * anim_pos), 0.0f);

			Color mid_colors[4] = { col1, col1, col2, col2 };
			FRect uv_rect(0, 0, 1, 1);

			renderer.addFilledRect(rects[0], col1);
			renderer.addFilledRect(rects[1], uv_rect, mid_colors, Color::white);
			renderer.addFilledRect(rects[2], col2);
		}
		renderer.render();

	}

	void Loop::draw() {
		onDraw();

		if(m_is_transitioning)
			m_transition.draw(IRect(GfxDevice::instance().windowSize()));
	}

}

