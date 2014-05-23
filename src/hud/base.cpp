/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/base.h"
#include "gfx/device.h"
#include "audio/device.h"

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

	void drawBorder(const FRect &rect, Color color, const float2 &offset, float width) {
		float2 min = rect.min - offset, max = rect.max + offset;
		width = ::min(width, (max.x - min.x) * 0.5f - 2.0f);
		Color transparent = Color(color, 0);

		DTexture::unbind();
	
		// left
		drawLine(float2(min.x, min.y), float2(min.x, max.y), color, color);
		drawLine(float2(min.x + width, min.y), float2(min.x, min.y), transparent, color);
		drawLine(float2(min.x + width, max.y), float2(min.x, max.y), transparent, color);
			
		// right
		drawLine(float2(max.x, min.y), float2(max.x, max.y), color, color);
		drawLine(float2(max.x - width, min.y), float2(max.x, min.y), transparent, color);
		drawLine(float2(max.x - width, max.y), float2(max.x, max.y), transparent, color);
	}
	
	const FRect align(const FRect &rect, const FRect &relative_to, Alignment mode, float spacing) {
		DASSERT(mode >= align_top && mode <= align_down_right);

		if(mode >= align_top_left) {
			if(mode == align_top_left)
				return align(align(rect, relative_to, align_left, spacing), relative_to, align_top, spacing);
			else if(mode == align_top_right)
				return align(align(rect, relative_to, align_right, spacing), relative_to, align_top, spacing);
			else if(mode == align_down_left)
				return align(align(rect, relative_to, align_left, spacing), relative_to, align_down, spacing);
			else /*if(mode == align_down_right)*/
				return align(align(rect, relative_to, align_right, spacing), relative_to, align_down, spacing);
		}

		if(mode == align_top) {
			float top = relative_to.min.y - spacing;
			return FRect(rect.min.x, top - rect.height(), rect.max.x, top);
		}
		else if(mode == align_down) {
			float bottom = relative_to.max.y + spacing;
			return FRect(rect.min.x, bottom, rect.max.x, bottom + rect.height());
		}
		else if(mode == align_left) {
			float left = relative_to.min.x - spacing;
			return FRect(left - rect.width(), rect.min.y, left, rect.max.y);
		}
		else /*if(mode == align_right)*/ {
			float right = relative_to.max.x + spacing;
			return FRect(right, rect.min.y, right + rect.width(), rect.max.y);
		}
	}
	
	void animateValue(float &value, float speed, bool maximize) {
		value = clamp(value, 0.0f, 1.0f);
		value += (maximize? 1.0f : -1.0f) * (1.0f - (value * value) * 0.9) * speed;
		value = clamp(value, 0.0f, 1.0f);
	}
	
	static const char *s_sound_names[(int)HudSound::count] = {
		"butn_text",
		"butn_itemswitch",
		"butn_optionknob"
	};

	void playSound(HudSound id) {
		DASSERT(id >= (HudSound)0 && id < HudSound::count);
		audio::playSound(s_sound_names[(int)id], 1.0f);
	}


	DEFINE_ENUM(HudStyleId,
		"Whiteish Green",
		"Greenish Red"
	);
		
	const float HudStyle::s_spacing = 15.0f;
	const float HudStyle::s_layer_spacing = 5.0f;

	HudStyle getStyle(HudStyleId::Type style_id) {
		DASSERT(HudStyleId::isValid(style_id));
		if(style_id == HudStyleId::green_white)
			return HudStyle{
				Color(150, 200, 150),
				Color(30, 255, 60),
				Color(30, 255, 60),
				Color::white,
				5.0f,
				"transformers_20",
				"transformers_30"
			};
		else //style_id == HudStyleId::red_green)
			return HudStyle{
				Color::green,
				Color(255, 60, 30),
				Color(255, 60, 30),
				Color::white,
				5.0f,
				"transformers_20",
				"transformers_30"
			};
	}
}

