// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "hud/widget.h"

namespace hud {

class HudGrid : public HudWidget {
  public:
	HudGrid(const FRect &rect);

	void addColumn(const char *title, float min_size);
	int numColumns() const { return (int)m_columns.size(); }

	void addRow(int id);
	void removeRow(int id);
	void clearRows();

	int selectedRow() const { return m_selected_row; }
	void selectRow(int idx);

	void setCell(int row_id, int col_id, const string &value);

	int numVisibleRows() const;
	int numRows() const;

	int scrollPos() const;
	void scroll(int offset);

  protected:
	bool onInput(const InputEvent &) override;
	void onDraw(Renderer2D &) const override;
	void onLayout() override;
	void onUpdate(double time_diff) override;

	struct Column {
		const char *title;
		float min_size;
		FRect rect;
	};

	struct Row {
		vector<string> cells;
		FRect rect;

		float highlighted_time = 0.0f;
		float selection_time = 0.0f;
	};

	vector<Column> m_columns;
	std::map<int, Row> m_rows;
	int m_scroll_pos = 0, m_max_visible_rows = 0;
	int m_selected_row = -1, m_highlighted_row = -1;
};

}
