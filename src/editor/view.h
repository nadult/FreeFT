// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include "game/entity_map.h"
#include "occluder_map.h"
#include <fwk/sys/input.h>

namespace ui {

class View {
  public:
	View(game::TileMap &, game::EntityMap &, const int2 &view_size);

	void drawGrid(Renderer2D &) const;
	void update(const InputState &);

	void setGridHeight(int new_height) { m_height = new_height; }
	int gridHeight() const { return m_height; }
	int cellSize() const { return m_cell_size; }
	bool isGridVisible() const { return m_is_visible; }

	const int2 &pos() const { return m_view_pos; }
	void updateVisibility(int cursor_height = 0);
	const IBox computeCursor(const int2 &start, const int2 &end, const int3 &bbox, int height,
							 int offset) const;

  private:
	game::TileMap &m_tile_map;
	game::EntityMap &m_entity_map;
	OccluderConfig m_occluder_config;

	int m_height, m_cell_size;
	bool m_is_visible;
	int2 m_view_pos;
	const int2 m_view_size;
};

using PView = Dynamic<View>;

}
