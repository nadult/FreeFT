/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/base.h"
#include "gfx/device.h"

using namespace gfx;

namespace hud
{

	void drawGradQuad(const FRect &rect, Color a, Color b, bool is_vertical) {
		Color colors[4] = { a, b, b, a };
		if(is_vertical)
			swap(colors[1], colors[3]);
		gfx::drawQuad(rect, FRect(0, 0, 1, 1), colors);
	}

	void drawLine(float2 p1, float2 p2, Color a, Color b) {
		if(p1.x > p2.x) {
			swap(p1.x, p2.x);
			swap(a, b);
		}
		if(p1.y > p2.y) {
			swap(p1.y, p2.y);
			swap(a, b);
		}

		bool is_vertical = false;
		if(p1.x == p2.x) {
			p1.x--;
			p2.x++;
			is_vertical = true;
		}
		else {
			p1.y--;
			p2.y++;
		}

		drawGradQuad(FRect(p1, p2), a, b, is_vertical);
	}

	void drawBorder(const FRect &rect, Color color, const float2 &offset, float width, bool is_left) {
		float2 min = rect.min - offset, max = rect.max + offset;
		width = ::min(width, (max.x - min.x) * 0.5f - 2.0f);

		DTexture::bind0();
		if(is_left) {
			drawLine(float2(min.x, min.y), float2(min.x, max.y), color, color);
			drawLine(float2(min.x + width, min.y), float2(min.x, min.y), Color::transparent, color);
			drawLine(float2(min.x + width, max.y), float2(min.x, max.y), Color::transparent, color);
		}
		else {
			drawLine(float2(max.x, min.y), float2(max.x, max.y), color, color);
			drawLine(float2(max.x - width, min.y), float2(max.x, min.y), Color::transparent, color);
			drawLine(float2(max.x - width, max.y), float2(max.x, max.y), Color::transparent, color);
		}
	}

	void drawLayer(const FRect &rect, Color color) {
		DTexture::bind0();
		drawQuad(rect, Color(color, 40));
		drawBorder(rect, Color(color, 100), float2(0, 0), 100.0f, false);
		drawBorder(rect, Color(color, 100), float2(0, 0), 100.0f, true);
	}

	DEFINE_ENUM(HudStyleId,
		"Blueish Green",
		"Greenish Red"
	);

	static HudStyle s_styles[HudStyleId::count] = {
		HudStyle{
			Color::blue,
			Color(30, 255, 60),
			Color(30, 255, 60),
			Color::white,
			5.0f,
			"transformers_20"
		},
		HudStyle{
			Color::green,
			Color(255, 60, 30),
			Color(255, 60, 30),
			Color::white,
			5.0f,
			"transformers_20"
		}
	};
	
	HudStyle getStyle(HudStyleId::Type style_id) {
		DASSERT(HudStyleId::isValid(style_id));
		return s_styles[style_id];
	}
}

