/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "editor/tile_selector.h"
#include "gfx/device.h"

using namespace gfx;
using game::Tile;

namespace ui {

	TileSelector::TileSelector(IRect rect) :Window(rect, Color::gui_dark),
		m_tile_list(rect.width(), 2), m_selection(nullptr) {
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
		
	void TileSelector::drawContents() const {
		int2 offset = innerOffset();
		IRect clip_rect(int2(0, 0), clippedRect().size());

		for(int n = 0; n < (int)m_tile_list.size(); n++) {
			const Tile *tile = m_tile_list[n].tile;
			IRect tile_rect = tile->rect();
			int2 pos = m_tile_list[n].pos - tile_rect.min - offset;

			if(areOverlapping(clip_rect, tile_rect + pos))
				tile->draw(pos);
		}
		
		DTexture::bind0();

		if(m_selection) {
			int2 pos = m_selection->pos - offset;

			lookAt(-clippedRect().min - pos + m_selection->tile->rect().min);
			IBox box(int3(0, 0, 0), m_selection->tile->bboxSize());
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
