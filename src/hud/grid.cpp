/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "hud/grid.h"
#include "gfx/drawing.h"

namespace hud {

	HudGrid::HudGrid(const FRect &rect)
		  :HudWidget(rect) {
		m_anim_speed = 5.0f;
	}
		
	void HudGrid::addColumn(const char *title, float min_size) {
		m_columns.emplace_back(Column{title, min_size, {}});
		needsLayout();
	}
		
	void HudGrid::addRow(int id) {
		DASSERT(id >= 0);
		m_rows[id] = Row();
		needsLayout();
	}
		
	void HudGrid::clearRows() {
		m_rows.clear();
		m_selected_row = -1;
		m_scroll_pos = 0;
		needsLayout();
	}

	void HudGrid::removeRow(int id) {
		DASSERT(id >= 0);
		m_rows.erase(id);
		if(id == m_selected_row)
			m_selected_row = -1;
		needsLayout();
	}
		
	void HudGrid::selectRow(int idx) {
		auto it = m_rows.find(idx);
		DASSERT(it != m_rows.end());
		it->second.selection_time = it->second.highlighted_time = 1.0f;
		m_selected_row = idx;
	}

	void HudGrid::setCell(int row_id, int col_id, const string &value) {
		auto it = m_rows.find(row_id);
		DASSERT(it != m_rows.end());
		DASSERT(col_id >= 0 && col_id < (int)m_columns.size());

		if(it->second.cells.size() < m_columns.size())
			it->second.cells.resize(m_columns.size());
		it->second.cells[col_id] = value;
		needsLayout();
	}
		
	int HudGrid::numVisibleRows() const {
		return m_max_visible_rows;
	}
		
	int HudGrid::numRows() const {
		return (int)m_rows.size();
	}

	int HudGrid::scrollPos() const {
		return m_scroll_pos;
	}

	void HudGrid::scroll(int offset) {
		m_scroll_pos += offset;
		m_scroll_pos = clamp(m_scroll_pos, 0, (int)m_rows.size() - 1);
		needsLayout();
	}
		
	bool HudGrid::onInput(const InputEvent &event) {
		bool mouse_key_down = event.mouseButtonDown(InputButton::left);

		if((mouse_key_down || event.isMouseOverEvent()) && isMouseOver(event)) {
			if(mouse_key_down)
				m_selected_row = -1;
			else
				m_highlighted_row = -1;

			for(const auto &row :m_rows)
				if(row.second.rect.isInside((float2)event.mousePos())) {
					if(mouse_key_down) {
						m_selected_row = row.first;
						handleEvent(this, HudEvent::row_clicked, m_selected_row);
					}
					else {
						m_highlighted_row = row.first;
					}
				}

			return mouse_key_down;
		}

		return false;
	}

	void HudGrid::onDraw(Renderer2D &out) const {
		for(int col = 0; col < (int)m_columns.size(); col++) {
			const Column &column = m_columns[col];
			FRect rect = column.rect;

			out.addFilledRect(rect, mulAlpha(lerp(Color::white, Color::green, col & 1? 0.3f : 0.6f), 0.5f));
			m_font->draw(out, rect, {Color::white, Color::black, HAlign::center, VAlign::top}, column.title);
		
			int counter = 0;	
			for(auto &row_it : m_rows) {
				int view_index = counter++ - m_scroll_pos;
				if(view_index < 0 || view_index >= m_max_visible_rows)
					continue;

				const Row &row = row_it.second;
				const string &cell_value = col > (int)row.cells.size()? string() : row.cells[col];
				m_font->draw(out, FRect(rect.min.x, row.rect.min.y, rect.max.x, row.rect.max.y),
							 {lerp(Color(200, 200, 200), Color::white, row.highlighted_time), Color::black, HAlign::center}, cell_value);
			}
		}

		for(auto &row_it : m_rows) {
			const Row &row = row_it.second;
			bool is_selected = row_it.first == m_selected_row;

			float2 offset = float2(50.0f, 50.0f) * (is_selected? 1.0f - row.selection_time : 0.0f);
			drawBorder(out, row.rect, mulAlpha(Color(200, 255, 200, 255), pow(row.selection_time, 2.0f)), offset, 500.0f);
		}
	}

	void HudGrid::onLayout() {
		FRect rect = this->rect();
		enum { spacing = layer_spacing };

		float size_left = rect.width(), min_sum = 0.0f;
		for(const auto &column : m_columns) {
			size_left -= column.min_size + spacing;
			min_sum += column.min_size;
		}

		float frac = 0.0f, pos = 0.0f;
		for(int col = 0; col < (int)m_columns.size(); col++) {
			float col_size = m_columns[col].min_size + size_left * (m_columns[col].min_size / min_sum) + frac;
			int isize = (int)col_size;
			frac = col_size - isize;
			m_columns[col].rect = FRect(rect.min.x + pos, rect.min.y, rect.min.x + pos + isize, rect.max.y);
			if(col == (int)m_columns.size() - 1)
				m_columns[col].rect.max.x = rect.max.x;
			pos += isize + spacing;
		}

		float row_size = m_font->lineHeight();
		m_max_visible_rows = (int)((rect.height() + spacing) / (row_size + spacing)) - 1;

		while(m_scroll_pos >= (int)m_rows.size())
			m_scroll_pos -= m_max_visible_rows;
		m_scroll_pos = max(0, m_scroll_pos);

		pos = rect.min.y + row_size + spacing;
		int counter = 0;
		for(auto &row : m_rows) {
			int view_index = counter++ - m_scroll_pos;
			if(view_index < 0 || view_index >= m_max_visible_rows) {
				row.second.rect = {};
				continue;
			}

			row.second.rect = FRect(rect.min.x, pos, rect.max.x, pos + row_size);
			pos += row_size + spacing;
		}
	}
		
	void HudGrid::onUpdate(double time_diff) {
		HudWidget::onUpdate(time_diff);
		for(auto &row_it : m_rows) {
			Row &row = row_it.second;
			animateValue(row.selection_time, time_diff * m_anim_speed, m_selected_row == row_it.first);
			animateValue(row.highlighted_time, time_diff * m_anim_speed, m_highlighted_row == row_it.first);
			row.highlighted_time = max(row.highlighted_time, row.selection_time);
		}
	}

}
