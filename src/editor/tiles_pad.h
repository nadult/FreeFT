// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "editor/tile_group.h"
#include "editor/tile_selector.h"
#include "editor/tiles_editor.h"
#include "ui/combo_box.h"
#include "ui/progress_bar.h"

namespace ui {

class TilesPad : public Window {
  public:
	TilesPad(const IRect &rect, PTilesEditor editor, TileGroup *group);

	TileFilter currentFilter() const;
	void updateTileList();
	void updateDirtyBar();
	virtual bool onEvent(const Event &ev);
	void updateEditor(PTilesEditor);

	TileGroup *m_group;

	PTilesEditor m_editor;
	PComboBox m_filter_box;
	PProgressBar m_dirty_bar;
	PTileSelector m_selector;

	PComboBox m_editor_mode_box;

	bool m_is_grouped_model;
};

using PTilesPad = shared_ptr<TilesPad>;

}
