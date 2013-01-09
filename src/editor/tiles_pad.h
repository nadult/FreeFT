#ifndef EDITOR_TILES_PAD_H
#define EDITOR_TILES_PAD_H

#include "editor/tiles_editor.h"
#include "editor/tile_selector.h"
#include "editor/tile_group.h"
#include "ui/combo_box.h"
#include "ui/progress_bar.h"

namespace ui {

	class TilesPad: public Window
	{
	public:
		TilesPad(const IRect &rect, PTilesEditor editor, TileGroup *group);

		TileFilter::Type currentFilter() const;
		void updateTileList();
		void updateDirtyBar();
		virtual bool onEvent(const Event &ev);

		TileGroup		*m_group;

		PTilesEditor	m_editor;
		PComboBox		m_filter_box;
		PProgressBar 	m_dirty_bar;
		PTileSelector	m_selector;

		TilesEditor::Mode m_editor_mode;
	};
	
	typedef Ptr<TilesPad> PTilesPad;

}

#endif
