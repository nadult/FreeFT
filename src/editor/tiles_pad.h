/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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

		PComboBox		m_editor_mode_box;

		bool			m_is_grouped_model;
	};
	
	typedef Ptr<TilesPad> PTilesPad;

}

#endif
