#include "ui/tile_selector.h"
#include "gfx/device.h"

using namespace gfx;



namespace ui {

	TileSelector::TileSelector(IRect rect) :Window(rect, Color::gui_dark),
		m_tile_list(rect.width(), 2), m_selection(nullptr) {
		update();
	}

	void TileSelector::setModel(PTileListModel model) {
		m_tile_list.setModel(model);
		setInnerRect(IRect(0, 0, rect().width(), m_tile_list.m_height));
	}

	void TileSelector::update() {
		m_tile_list.update();
		setInnerRect(IRect(0, 0, rect().width(), m_tile_list.m_height));
	}
		
	void TileSelector::setSelection(const gfx::Tile *tile) {
		m_selection = nullptr;

		for(int n = 0; n < m_tile_list.size(); n++)
			if(m_tile_list[n].tile == tile) {
				m_selection = &m_tile_list[n];
				return;
			}
	}
		
	void TileSelector::drawContents() const {
		int2 offset = innerOffset();

		for(int n = 0; n < (int)m_tile_list.size(); n++) {
			const Tile *tile = m_tile_list[n].tile;
			tile->draw(m_tile_list[n].pos - tile->GetBounds().min - offset);
		}
		
		DTexture::bind0();

		if(m_selection) {
			int2 pos = m_selection->pos - offset;

			lookAt(-clippedRect().min - pos - m_selection->tile->m_offset);
			IBox box(int3(0, 0, 0), m_selection->tile->m_bbox);
			drawBBox(box);
		//	drawRect(IRect(pos, pos + m_selection->size));
		}
	}
		
	bool TileSelector::onMouseDrag(int2 start, int2 current, int key, int is_final) {
		if(key == 0) {
			m_selection = m_tile_list.find(current + innerOffset());
			sendEvent(this, Event::element_selected);
			return true;
		}

		return false;
	}

}
