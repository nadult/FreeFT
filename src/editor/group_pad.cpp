// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "editor/group_pad.h"

namespace ui {
GroupPad::GroupPad(const IRect &rect, PGroupEditor editor, TileGroup *group)
	: Window(rect), m_editor(editor), m_group(group) {
	EnumMap<TileFilter, const char *> strings;
	for(auto e : all<TileFilter>)
		strings[e] = toString(e);
	m_filter_box = make_shared<ComboBox>(IRect(0, 0, rect.width(), 22), 200, "Filter: ", strings);
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
