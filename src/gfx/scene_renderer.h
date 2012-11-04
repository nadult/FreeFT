#ifndef GFX_SCENE_RENDERER_H
#define GFX_SCENE_RENDERER_H

#include "gfx/device.h"

namespace gfx {

	class SceneRenderer
	{
	public:
		SceneRenderer(IRect viewport, int2 view_pos);
		void add(PTexture tex, IRect rect, float3 pos, int3 bbox);
		void addBBox(IBox box); //TODO: colors
		void render();

		IRect targetRect() const { return m_viewport + m_view_pos; }

	protected:
		struct Element {
			bool operator<(const Element &rhs) const;

			//TODO: weak ptr to texture, make sure that textures exist
			PTexture m_texture;
			IRect m_rect;
			float3 m_pos;
			int3 m_bbox;
		};

		vector<Element> m_elements;
		vector<IBox> m_bboxes;

		IRect m_viewport;
		int2 m_view_pos;
	};

}

#endif
