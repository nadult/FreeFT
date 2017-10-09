/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/base.h"
#include "audio/device.h"
#include "gfx/drawing.h"

using namespace gfx;

namespace hud
{

	void drawGradQuad(Renderer2D &out, const FRect &rect, Color a, Color b, bool is_vertical) {
		FColor colors[4] = { a, a, b, b };
		if(is_vertical)
			swap(colors[1], colors[3]);
		out.addFilledRect(rect, FRect(0, 0, 1, 1), colors, FColor(ColorId::white));
	}

	void drawLine(Renderer2D &out, float2 p1, float2 p2, Color a, Color b) {
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

		drawGradQuad(out, FRect(p1, p2), a, b, is_vertical);
	}

	void drawBorder(Renderer2D &out, const FRect &rect, Color color, const float2 &offset, float width) {
		float2 min = rect.min() - offset, max = rect.max() + offset;
		width = fwk::min(width, (max.x - min.x) * 0.5f - 2.0f);
		Color transparent = Color(color, 0);

		DTexture::unbind();
	
		// left
		drawLine(out, float2(min.x, min.y), float2(min.x, max.y), color, color);
		drawLine(out, float2(min.x + width, min.y), float2(min.x, min.y), transparent, color);
		drawLine(out, float2(min.x + width, max.y), float2(min.x, max.y), transparent, color);
			
		// right
		drawLine(out, float2(max.x, min.y), float2(max.x, max.y), color, color);
		drawLine(out, float2(max.x - width, min.y), float2(max.x, min.y), transparent, color);
		drawLine(out, float2(max.x - width, max.y), float2(max.x, max.y), transparent, color);
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
			float top = relative_to.y() - spacing;
			return FRect(rect.x(), top - rect.height(), rect.ex(), top);
		}
		else if(mode == align_down) {
			float bottom = relative_to.ey() + spacing;
			return FRect(rect.x(), bottom, rect.ex(), bottom + rect.height());
		}
		else if(mode == align_left) {
			float left = relative_to.x() - spacing;
			return FRect(left - rect.width(), rect.y(), left, rect.ey());
		}
		else /*if(mode == align_right)*/ {
			float right = relative_to.ex() + spacing;
			return FRect(right, rect.y(), right + rect.width(), rect.ey());
		}
	}
	
	void animateValue(float &value, float speed, bool maximize) {
		value = clamp(value, 0.0f, 1.0f);
		value += (maximize? 1.0f : -1.0f) * (1.0f - (value * value) * 0.9) * speed;
		value = clamp(value, 0.0f, 1.0f);
	}

	static EnumMap<HudSound, const char*> s_sounds = {
		"",
		"butn_text",
		"butn_itemswitch",
		"butn_optionknob"
	};

	void playSound(HudSound id) {
		if(id != HudSound::none)
			audio::playSound(s_sounds[id]);
	}

	HudStyle getStyle(HudStyleId style_id) {
		if(style_id == HudStyleId::green_white)
			return HudStyle{	Color(150, 200, 150),	Color(30, 255, 60),		Color(30, 255, 60),		ColorId::white,
								"transformers_20",		"transformers_30" };
		else if(style_id == HudStyleId::red_green)
			return HudStyle{	ColorId::green,			Color(255, 60, 30),		Color(255, 60, 30),		ColorId::white,
								"transformers_20",		"transformers_30" };
		else //if(style_id == HudStyleId::console)
			return HudStyle{	ColorId::green,			Color(255, 60, 30),		Color(255, 60, 30),		ColorId::white,
								"liberation_16",		"liberation_24"   };
	}
}

