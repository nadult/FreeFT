/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GFX_SCENE_RENDERER_H
#define GFX_SCENE_RENDERER_H

#include "base.h"

class SceneRenderer {
  public:
	SceneRenderer(IRect viewport, int2 view_pos);
	bool add(STexture tex, IRect rect, float3 pos, FBox bbox, Color col = Color::white,
			 FRect tex_rect = FRect(0, 0, 1, 1), bool is_overlay = false);
	bool add(STexture tex, IRect rect, float3 pos, int3 bbox, Color col = Color::white,
			 FRect tex_rect = FRect(0, 0, 1, 1), bool is_overlay = false) {
		return add(tex, rect, pos, FBox(float3(0, 0, 0), float3(bbox)), col, tex_rect, is_overlay);
	}

	void addBox(FBox box, Color col = Color::white, bool is_filled = false);
	void addBox(IBox box, Color col = Color::white, bool is_filled = false) {
		addBox((FBox)box, col, is_filled);
	}
	void addLine(int3, int3, Color = Color::white);
	void render();

	const IRect &targetRect() const { return m_target_rect; }

  protected:
	struct Element {
		STexture texture;
		IRect rect;
		FBox bbox;
		FRect tex_rect;
		Color color;
		bool is_overlay;
	};

	struct BoxElement {
		FBox bbox;
		Color color;
	};

	struct LineElement {
		int3 begin, end;
		Color color;
	};

	vector<Element> m_elements; // filled boxes go here (with texture == nullptr)
	vector<BoxElement> m_wire_boxes;
	vector<LineElement> m_lines;

	IRect m_viewport, m_target_rect;
	int2 m_view_pos;
};

#endif
