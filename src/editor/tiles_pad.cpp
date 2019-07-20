// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "editor/tiles_pad.h"
#include <algorithm>

namespace ui {

	TilesPad::TilesPad(const IRect &rect, PTilesEditor editor, TileGroup *group)
		:Window(rect, ColorId::transparent), m_editor(editor), m_group(group) {
		int width = rect.width();

		auto strings = transform(all<TileFilter>(), [](TileFilter f) { return toString(f); });
		m_filter_box = make_shared<ComboBox>(IRect(0, 0, width/2, 22), 200, "Filter: ", strings);
		m_filter_box->selectEntry(0);
		m_dirty_bar = make_shared<ProgressBar>(IRect(width/2, 0, width, 22), true);
		m_editor_mode_box = make_shared<ComboBox>(IRect(0, 22, width, 44), 200, "Editing mode: ",
				TilesEditor::modeStrings());
		
		m_selector = make_shared<TileSelector>(IRect(0, 44, width, rect.height()));
		
		m_is_grouped_model = false;
		updateTileList();

		attach(m_filter_box);
		attach(m_dirty_bar);
		attach(m_editor_mode_box);
		attach(m_selector);

		updateDirtyBar();
	}

	TileFilter TilesPad::currentFilter() const {
		TileFilter filter = (TileFilter)m_filter_box->selectedId();
		DASSERT(validEnum(filter));
		return filter;
	}

	void TilesPad::updateTileList() {
		PTileListModel model = m_is_grouped_model?
			groupedTilesModel(*m_group, m_editor->isFilling()) : allTilesModel();
		model = filteredTilesModel(model, currentFilter());
		m_selector->setModel(model);
	}

	void TilesPad::updateDirtyBar() {
		char text[64];
		snprintf(text, sizeof(text), "Dirty tiles: %d%%", (int)(m_dirty_bar->pos() * 100));
		m_dirty_bar->setText(text);
		m_editor->m_dirty_percent = m_dirty_bar->pos();
	}
	
	bool TilesPad::onEvent(const Event &ev) {
		if(ev.type == Event::progress_bar_moved && m_dirty_bar.get() == ev.source)
			updateDirtyBar();
		else if(ev.type == Event::button_clicked && m_editor.get() == ev.source) {
			if(m_editor_mode_box->selectedId() != m_editor->mode())
				m_editor_mode_box->selectEntry(m_editor->mode());
			bool is_grouped_model = m_editor->isPlacingRandom() || m_editor->isFilling();
			if(is_grouped_model != m_is_grouped_model) {
				m_is_grouped_model = is_grouped_model;
				updateTileList();
			}
		}
		else if(ev.type == Event::element_selected && m_selector.get() == ev.source) {
			//TODO: print tile name in selector
			//printf("new tile: %s\n", m_selector->selection()? m_selector->selection()->name.c_str() : "none");
			m_editor->setNewTile(m_selector->selection());
		}
		else if(ev.type == Event::element_selected && m_filter_box.get() == ev.source)
			updateTileList();
		else if(ev.type == Event::element_selected && m_editor_mode_box.get() == ev.source)
			m_editor->setMode((TilesEditor::Mode)ev.value);
		else
			return false;

		return true;
	}


}
