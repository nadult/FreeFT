/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef GFX_SCENE_RENDERER_H
#define GFX_SCENE_RENDERER_H

#include "gfx/device.h"

namespace gfx {

	class SceneRenderer
	{
	public:
		SceneRenderer(IRect viewport, int2 view_pos);
		void add(PTexture tex, IRect rect, float3 pos, FBox bbox, Color col = Color::white, FRect tex_rect = FRect(0, 0, 1, 1));
		void add(PTexture tex, IRect rect, float3 pos, int3 bbox, Color col = Color::white, FRect tex_rect = FRect(0, 0, 1, 1))
			{ add(tex, rect, pos, FBox(float3(0, 0, 0), bbox), col, tex_rect); }

		void addBox(FBox box, Color col = Color::white, bool is_filled = false);
		void addBox(IBox box, Color col = Color::white, bool is_filled = false)
			{ addBox((FBox)box, col, is_filled); }
		void addLine(int3, int3, Color = Color::white);
		void render();

		const IRect &targetRect() const { return m_target_rect; }

	protected:
		struct Element {
			//TODO: change to weak ptr to texture, make sure that textures exist
			PTexture texture;
			IRect rect;
			FBox bbox;
			FRect tex_rect;
			Color color;
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

}

#endif
