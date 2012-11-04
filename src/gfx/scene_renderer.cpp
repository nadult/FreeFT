#include "gfx/scene_renderer.h"
#include "gfx/device.h"
#include <algorithm>

namespace gfx {
			
	bool SceneRenderer::Element::operator<(const SceneRenderer::Element &rhs) const {
		IBox box[2] = {
			IBox((int3)m_pos, (int3)m_pos + m_bbox),
			IBox((int3)rhs.m_pos, (int3)rhs.m_pos + rhs.m_bbox) };

		//DAssert(!Overlaps(box, box));

		if(box[0].max.y <= box[1].min.y)
			return true;
		if(box[1].max.y <= box[0].min.y)
			return false;
		
		if(box[0].max.x <= box[1].min.x)
			return true;
		if(box[1].max.x <= box[0].min.x)
			return false;

		if(box[0].max.z <= box[1].min.z)
			return true;
		if(box[1].max.z <= box[0].min.z)
			return false;

		DAssert(Overlaps(box[0], box[1]));
		return false;
	}

	SceneRenderer::SceneRenderer(IRect viewport, int2 view_pos) :m_viewport(viewport), m_view_pos(view_pos) {
	}

	void SceneRenderer::add(PTexture texture, IRect rect, float3 pos, int3 bbox) {
		Element new_elem;
		new_elem.m_texture = texture;
		new_elem.m_bbox = bbox;
		new_elem.m_pos  = pos;
		new_elem.m_rect = rect;
		m_elements.push_back(new_elem);
	}

	void SceneRenderer::addBBox(IBox bbox) {
		//TODO: rozbic bboxy na poszczegolne linie i dodac jako normalne elementy
		m_bboxes.push_back(bbox);
	}

	void SceneRenderer::render() {
		SetScissorTest(true);
		SetScissorRect(m_viewport);	
		LookAt(m_view_pos - m_viewport.min);
		IRect view(m_view_pos, m_view_pos + m_viewport.Size());

		//TODO: podzielic ekran na male kawalki, do kazdego z nich przyporzadkowac widoczne
		// elementy, i rysowac (i sortowac) kawalki niezaleznie z wlaczonym scissor rectem
		// dzieki temu zminimalizujemy cykle w porzadku wyswietlania pmiedzy poszczegolnymi elementami
		std::stable_sort(m_elements.begin(), m_elements.end());

		for(int n = 0; n < (int)m_elements.size(); n++) {
			const Element &elem = m_elements[n];
			elem.m_texture->Bind();
			int2 screen_pos = (int2)WorldToScreen(elem.m_pos).xy();

			if(Overlaps(view, elem.m_rect + screen_pos))
				DrawQuad(screen_pos + elem.m_rect.min, elem.m_rect.Size());
		}

		DTexture::Bind0();
		for(int n = 0; n < (int)m_bboxes.size(); n++)
			DrawBBox(m_bboxes[n]);

		SetScissorTest(false);
	}

}
