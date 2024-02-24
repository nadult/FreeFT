// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "editor/tile_selector.h"
#include "gfx/drawing.h"

using game::Tile;

namespace ui {

TileSelector::TileSelector(IRect rect)
	: Window(rect, WindowStyle::gui_dark), m_tile_list(rect.width(), 2), m_selection(nullptr) {
	update();
}

void TileSelector::setModel(PTileListModel model) {
	m_tile_list.setModel(model);
	setInnerRect(IRect(0, 0, rect().width(), m_tile_list.m_height));
	m_selection = nullptr;
}

void TileSelector::update() {
	m_tile_list.update();
	setInnerRect(IRect(0, 0, rect().width(), m_tile_list.m_height));
}

void TileSelector::setSelection(const game::Tile *tile) {
	m_selection = nullptr;

	for(int n = 0; n < m_tile_list.size(); n++)
		if(m_tile_list[n].tile == tile) {
			m_selection = &m_tile_list[n];
			return;
		}
}

void TileSelector::drawContents(Renderer2D &out) const {
	int2 offset = innerOffset();
	IRect clip_rect(int2(0, 0), clippedRect().size());

	for(int n = 0; n < (int)m_tile_list.size(); n++) {
		const Tile *tile = m_tile_list[n].tile;
		IRect tile_rect = tile->rect();
		int2 pos = m_tile_list[n].pos - tile_rect.min() - offset;

		if(areOverlapping(clip_rect, tile_rect + pos))
			tile->draw(out, pos);
	}

	if(m_selection) {
		int2 pos = m_selection->pos - offset;

		out.setViewPos(-clippedRect().min() - pos + m_selection->tile->rect().min());
		IBox box(int3(0, 0, 0), m_selection->tile->bboxSize());
		drawBBox(out, box);
		//	out.addFilledRect(IRect(pos, pos + m_selection->size));
	}
}

bool TileSelector::onMouseDrag(const InputState &, int2 start, int2 current, int key,
							   int is_final) {
	if(key == 0) {
		m_selection = m_tile_list.find(current + innerOffset());
		sendEvent(this, Event::element_selected);
		return true;
	}

	return false;
}

}
