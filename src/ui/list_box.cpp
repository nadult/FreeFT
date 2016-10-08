/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "ui/list_box.h"

using namespace gfx;

namespace ui {


	ListBox::ListBox(const IRect &rect, FColor color) :Window(rect, color), m_over_id(-1), m_dragging_id(-1) {
		m_font = res::getFont(WindowStyle::fonts[0]);
		m_line_height = m_font->lineHeight();
	}
		
	void ListBox::drawContents(Renderer2D &out) const {
		int2 offset = innerOffset();
		int2 vis_entries = visibleEntriesIds();
		
		for(int n = vis_entries.x; n < vis_entries.y; n++) {
			const Entry &entry = m_entries[n];
			IRect rect = entryRect(n) - offset;

			out.addLine(int2(rect.min.x, rect.max.y), rect.max, WindowStyle::gui_medium);

			FColor col = ColorId::transparent;
			
			if((m_over_id == n && m_dragging_id == -1) || m_dragging_id == n)
				col = WindowStyle::gui_medium;
			if((m_dragging_id == n && m_over_id == n) || entry.is_selected)
				col = WindowStyle::gui_light;

			if(col.a > 0.0f)
				out.addFilledRect(rect, col);
		}
		
		if(isPopup())
			out.addRect(IRect(1, 0, width(), height() - 1), backgroundColor());

		for(int n = vis_entries.x; n < vis_entries.y; n++) {
			int2 pos = int2(0, m_line_height * n) - offset;
			m_font->draw(out, (float2)(pos + int2(5, 0)), {m_entries[n].color, ColorId::black}, m_entries[n].text);
		}
	}

	int2 ListBox::visibleEntriesIds() const {
		int2 offset = innerOffset();
		int begin = max(0, offset.y / m_line_height);
		int end   = min((offset.y + rect().height() + m_line_height) / m_line_height, (int)m_entries.size());

		return int2(begin, end);
	}

	int ListBox::entryId(int2 pos) const {
		int2 vis_entries = visibleEntriesIds();
		pos += innerOffset();

		for(int n = vis_entries.x; n < vis_entries.y; n++)
			if(entryRect(n).isInside(pos))
				return n;
		return -1;
	}
		
	bool ListBox::onEvent(const Event &ev) {
		if(ev.type == Event::escape && isPopup()) {
			close(0);
			return true;
		}

		return false;
	}

	void ListBox::onInput(const InputState &state) {
		m_over_id = entryId(state.mousePos() - clippedRect().min);
	}

	bool ListBox::onMouseDrag(const InputState&, int2 start, int2 end, int key, int is_final) {
		if(key == 0) {
			m_over_id = entryId(end);
			m_dragging_id = entryId(start);

			if(is_final == 1 && m_over_id == m_dragging_id) {
				bool do_select = localRect().isInside(end) && (!isPopup() || (m_over_id >= 0 && m_over_id < size()));

				if(do_select)
					selectEntry(m_over_id);
				sendEvent(this, Event::element_clicked, m_over_id);
				if(isPopup())
					close(do_select?1 : 0);
			}
			if(is_final)
				m_dragging_id = -1;

			return true;
		}

		return false;
	}

	IRect ListBox::entryRect(int entry_id) const {
		int pos = entry_id * m_line_height;
		return IRect(0, pos, rect().width(), pos + m_line_height);
	}

	void ListBox::addEntry(const char *text, FColor col) {
		m_entries.push_back(Entry{col, text, false});
		update();
	}

	int ListBox::findEntry(const char *text) const {
		for(int n = 0; n < (int)m_entries.size(); n++)
			if(m_entries[n].text == text)
				return n;
		return -1;
	}

	void ListBox::clear() {
		m_entries.clear();
		update();
	}

	void ListBox::update() {
		setInnerRect(IRect(0, 0, rect().width(), m_font->lineHeight() * (int)m_entries.size()));
	}

	int ListBox::selectedId() const {
		for(int n = 0; n < (int)m_entries.size(); n++)
			if(m_entries[n].is_selected)
				return n;
		return -1;
	}

	void ListBox::selectEntry(int id) {
		for(int n = 0; n < (int)m_entries.size(); n++)
			m_entries[n].is_selected = n == id;
		sendEvent(this, Event::element_selected, id < 0 || id >= (int)m_entries.size()? -1 : id);
	}

}
