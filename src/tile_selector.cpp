#include "tile_selector.h"
#include "gfx/device.h"

using namespace gfx;

TileSelector::TileSelector(IRect rect) :Window(rect, Color(60, 60, 60)), m_offset(0, 0),
	m_tile_list(rect.width(), 2), m_selection(nullptr) {
}

void TileSelector::setModel(ui::PTileListModel model) {
	m_tile_list.setModel(model);
}

void TileSelector::update() {
	m_tile_list.update();
}
	
void TileSelector::setSelection(const gfx::Tile *tile) {
	m_selection = nullptr;

	for(int n = 0; n < m_tile_list.size(); n++)
		if(m_tile_list[n].m_tile == tile) {
			m_selection = &m_tile_list[n];
			return;
		}
}
	
void TileSelector::drawContents() const {
	for(int n = 0; n < (int)m_tile_list.size(); n++) {
		const Tile *tile = m_tile_list[n].m_tile;
		tile->draw(m_tile_list[n].m_pos - tile->GetBounds().min - m_offset);
	}
	
	DTexture::bind0();

	if(m_selection) {
		int2 pos = m_selection->m_pos - m_offset;

		lookAt(-clippedRect().min - pos - m_selection->m_tile->offset);
		IBox box(int3(0, 0, 0), m_selection->m_tile->bbox);
		drawBBox(box, Color(255, 255, 255));
	//	drawRect(IRect(pos, pos + m_selection->m_size));
	}
}
	
void TileSelector::onInput(int2 mouse_pos) {
	int wheel = getMouseWheelMove();
	if(wheel)
		m_offset.y -= wheel * rect().height() / 16;
	if(isKeyDown(Key_pageup))
		m_offset.y -= rect().height();
	if(isKeyDown(Key_pagedown))
		m_offset.y += rect().height();

	m_offset.y = clamp(m_offset.y, 0, m_tile_list.m_height);
}

bool TileSelector::onMouseDrag(int2 start, int2 current, int key, bool is_final) {
	if(key == 2 || (isKeyPressed(Key_lctrl) && key == 0)) {
		m_offset.y -= getMouseMove().y;
		m_offset.y = clamp(m_offset.y, 0, m_tile_list.m_height - clippedRect().height());
		return true;
	}
	else if(key == 0) {
		m_selection = m_tile_list.find(current + m_offset);
		return true;
	}

	return false;
}

