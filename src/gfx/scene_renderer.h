#ifndef GFX_SCENE_RENDERER_H
#define GFX_SCENE_RENDERER_H

#include "gfx/device.h"

namespace gfx {

	class SceneRenderer
	{
	public:
		SceneRenderer(IRect viewport, int2 view_pos);
		void add(PTexture tex, IRect rect, float3 pos, int3 bbox, Color col = Color(255, 255, 255));
		void addBox(IBox box, Color col = Color(255, 255, 255), bool is_filled = false);
		void render();

		IRect targetRect() const { return IRect(m_view_pos, m_view_pos + m_viewport.size()); }

		enum { node_size = 128 };

	protected:
		struct Element {
			int Compare(const Element &rhs) const;

			//TODO: change to weak ptr to texture, make sure that textures exist
			PTexture m_texture;
			IRect m_rect;
			IBox m_bbox;
			Color m_color;
		};

		struct BoxElement {
			IBox m_bbox;
			Color m_color;
		};

		vector<Element> m_elements; // filled boxes go here (with m_texture == nullptr)
		vector<BoxElement> m_wire_boxes;

		IRect m_viewport;
		int2 m_view_pos;
	};

}

#endif
