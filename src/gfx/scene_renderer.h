#ifndef GFX_SCENE_RENDERER_H
#define GFX_SCENE_RENDERER_H

#include "gfx/device.h"

namespace gfx {

	class SceneRenderer
	{
	public:
		SceneRenderer(IRect viewport, int2 view_pos);
		void add(PTexture tex, IRect rect, float3 pos, FBox bbox, Color col = Color::white);
		void add(PTexture tex, IRect rect, float3 pos, int3 bbox, Color col = Color::white)
			{ add(tex, rect, pos, FBox(float3(0, 0, 0), bbox), col); }

		void addBox(FBox box, Color col = Color::white, bool is_filled = false);
		void addBox(IBox box, Color col = Color::white, bool is_filled = false)
			{ addBox((FBox)box, col, is_filled); }
		void addLine(int3, int3, Color = Color::white);
		void render();

		IRect targetRect() const { return IRect(m_view_pos, m_view_pos + m_viewport.size()); }

		enum { node_size = 128 };

	protected:
		struct Element {
			int Compare(const Element &rhs) const;

			//TODO: change to weak ptr to texture, make sure that textures exist
			PTexture texture;
			IRect rect;
			FBox bbox;
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

		IRect m_viewport;
		int2 m_view_pos;
	};

}

#endif
