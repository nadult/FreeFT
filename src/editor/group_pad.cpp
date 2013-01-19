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

#include "editor/group_pad.h"

namespace ui {

	GroupPad::GroupPad(const IRect &rect, PGroupEditor editor, TileGroup *group)
		:Window(rect), m_editor(editor), m_group(group) {
		m_filter_box = new ComboBox(IRect(0, 0, rect.width(), 22), 200,
				"Filter: ", TileFilter::strings(), TileFilter::count);
		attach(m_filter_box.get());
		m_filter_box->selectEntry(editor->tileFilter());
	}

	TileFilter::Type GroupPad::currentFilter() const {
		TileFilter::Type filter = (TileFilter::Type)m_filter_box->selectedId();
		DASSERT(filter >= 0 && filter < TileFilter::count);
		return filter;
	}

	bool GroupPad::onEvent(const Event &ev) {
		if(ev.type == Event::element_selected && m_filter_box.get() == ev.source)
			m_editor->setTileFilter(currentFilter());
		else
			return false;

		return true;
	}



}
