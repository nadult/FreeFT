/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "editor/group_pad.h"

namespace ui {

	GroupPad::GroupPad(const IRect &rect, PGroupEditor editor, TileGroup *group)
		:Window(rect), m_editor(editor), m_group(group) {
		m_filter_box = make_shared<ComboBox>(IRect(0, 0, rect.width(), 22), 200,
				"Filter: ", enumStrings(TileFilter()));
		attach(m_filter_box);
		m_filter_box->selectEntry((int)editor->tileFilter());
	}

	TileFilter GroupPad::currentFilter() const {
		TileFilter filter = (TileFilter)m_filter_box->selectedId();
		DASSERT(validEnum(filter));
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
