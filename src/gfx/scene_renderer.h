// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"

#include <fwk/vulkan/vulkan_image.h>
#include <fwk/vulkan_base.h>

class SceneRenderer {
  public:
	SceneRenderer(IRect viewport, int2 view_pos);
	bool add(PVImageView tex, IRect rect, float3 pos, FBox bbox, Color col = ColorId::white,
			 FRect tex_rect = FRect(0, 0, 1, 1), bool is_overlay = false);
	bool add(PVImageView tex, IRect rect, float3 pos, int3 bbox, Color col = ColorId::white,
			 FRect tex_rect = FRect(0, 0, 1, 1), bool is_overlay = false) {
		return add(tex, rect, pos, FBox(float3(0, 0, 0), float3(bbox)), col, tex_rect, is_overlay);
	}

	void addBox(FBox box, Color col = ColorId::white, bool is_filled = false);
	void addBox(IBox box, Color col = ColorId::white, bool is_filled = false) {
		addBox((FBox)box, col, is_filled);
	}
	void addLine(int3, int3, Color = ColorId::white);
	void render(Canvas2D &);

	const IRect &targetRect() const { return m_target_rect; }

  protected:
	struct Element {
		PVImageView texture;
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
