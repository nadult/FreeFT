// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include "game/tile.h"
#include "ui/tile_list.h"
#include "ui/window.h"

class TileGroup;

namespace ui {

class TileSelector : public Window {
  public:
	TileSelector(IRect rect);

	void drawContents(Canvas2D &) const override;
	bool onMouseDrag(const InputState &, int2 start, int2 current, int key, int is_final) override;

	void setModel(PTileListModel);
	void update(); // call every time model changes

	const game::Tile *selection() const { return m_selection ? m_selection->tile : nullptr; }
	void setSelection(const game::Tile *);

  protected:
	TileList m_tile_list;
	const TileList::Entry *m_selection;
};

using PTileSelector = shared_ptr<TileSelector>;

}
