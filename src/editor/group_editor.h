/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFt.

   FreeFt is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFt is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef EDITOR_GROUP_EDITOR_H
#define EDITOR_GROUP_EDITOR_H

#include "base.h"
#include "ui/window.h"
#include "ui/tile_list.h"

class TileGroup;

namespace ui {

	class GroupEditor: public ui::Window {
	public:
		GroupEditor(IRect rect);

		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 current, int key, int is_final);
		virtual void drawContents() const;

		void setTarget(TileGroup* tile_group);
		void setTileFilter(TileFilter::Type);
		TileFilter::Type tileFilter() const { return m_tile_filter; }

	protected:
		void updateSelector();

		ui::TileList m_tile_list;
		TileGroup *m_tile_group;

		gfx::PFont m_font;
		IRect m_view;

		TileFilter::Type m_tile_filter;

		int2 m_offset[3];
		const ui::TileList::Entry *m_current_entry;
		int m_selected_group_id;
		int m_selected_surface_id;
		int m_select_mode;
		int m_selection_mode;
		enum {
			mAddRemove, // add / remove tiles to / from group
			mModify, // modify tiles relations
		} m_mode;
	};

	typedef Ptr<GroupEditor> PGroupEditor;

}


#endif
