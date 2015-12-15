/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef HUD_GRID_H
#define HUD_GRID_H

#include "hud/widget.h"

namespace hud {

	class HudGrid: public HudWidget {
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
		bool onInput(const InputEvent&) override;
		void onDraw(Renderer2D&) const override;
		void onLayout() override;
		void onUpdate(double time_diff) override;

		struct Column {
			const char *title;
			float min_size;
			FRect rect;
		};

		struct Row {
			Row();

			vector<string> cells;
			FRect rect;

			float highlighted_time;
			float selection_time;
		};

		vector<Column> m_columns;
		std::map<int, Row> m_rows;
		int m_scroll_pos, m_max_visible_rows;
		int m_selected_row, m_highlighted_row;
	};

}

#endif
