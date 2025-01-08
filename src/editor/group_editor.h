// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include "ui/tile_list.h"
#include "ui/window.h"

class TileGroup;

namespace ui {

class GroupEditor : public ui::Window {
  public:
	GroupEditor(IRect rect);

	void drawContents(Canvas2D &) const override;
	void onInput(const InputState &) override;
	bool onMouseDrag(const InputState &, int2 start, int2 current, int key, int is_final) override;

	void setTarget(TileGroup *tile_group);
	void setTileFilter(TileFilter);
	TileFilter tileFilter() const { return m_tile_filter; }

  protected:
	void updateSelector();

	ui::TileList m_tile_list;
	TileGroup *m_tile_group;

	const Font &m_font;
	IRect m_view;

	TileFilter m_tile_filter;

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

using PGroupEditor = shared_ptr<GroupEditor>;

}
