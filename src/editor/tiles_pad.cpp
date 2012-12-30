#include "editor/tiles_pad.h"
#include <algorithm>

namespace ui {

	TilesPad::TilesPad(const IRect &rect, PTilesEditor editor, TileGroup *group)
		:Window(rect, Color::transparent), m_editor(editor), m_group(group) {
		int width = rect.width();

		m_filter_box = new ComboBox(IRect(0, 0, width/2, 22), 200,
				"Filter: ", TileFilter::strings(), TileFilter::count);
		m_filter_box->selectEntry(0);
		m_dirty_bar = new ProgressBar(IRect(width/2, 0, width, 22), true);
		m_selector = new TileSelector(IRect(0, 22, width, rect.height()));
		
		m_editor_mode = TilesEditor::mode_placing;
		updateTileList();

		attach(m_filter_box.get());
		attach(m_dirty_bar.get());
		attach(m_selector.get());

		updateDirtyBar();
	}

	TileFilter::Type TilesPad::currentFilter() const {
		TileFilter::Type filter = (TileFilter::Type)m_filter_box->selectedId();
		DASSERT(filter >= 0 && filter < TileFilter::count);
		return filter;
	}

	void TilesPad::updateTileList() {
		PTileListModel model = m_editor_mode == TilesEditor::mode_placing? allTilesModel() :
			groupedTilesModel(*m_group, m_editor_mode == TilesEditor::mode_filling);
		model = filteredTilesModel(model, TileFilter::test, currentFilter());
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
			if(m_editor->m_mode != TilesEditor::mode_selecting && m_editor_mode != m_editor->m_mode) {
				m_editor_mode = m_editor->m_mode;
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
		else
			return false;

		return true;
	}


}
