#include "ui/list_view.h"

using namespace gfx;

namespace ui {


	ListView::ListView(const IRect &rect) :Window(rect, Color::gui_dark), m_over_id(-1), m_dragging_id(-1) {
		m_font = Font::mgr["arial_16"];
		ASSERT(m_font);
		m_line_height = m_font->lineHeight();
	}
		
	void ListView::drawContents() const {
		int2 offset = innerOffset();
		int2 vis_entries = visibleEntriesIds();

		DTexture::bind0();
		for(int n = vis_entries.x; n < vis_entries.y; n++) {
			const Entry &entry = m_entries[n];
			IRect rect = entryRect(n) - offset;

			drawLine(int2(rect.min.x, rect.max.y), rect.max, Color::gui_medium);

			Color col = Color::transparent;
			
			if((m_over_id == n && m_dragging_id == -1) || m_dragging_id == n)
				col = Color::gui_medium;
			if((m_dragging_id == n && m_over_id == n) || entry.is_selected)
				col = Color::gui_light;

			if(col != Color(Color::transparent))
				drawQuad(rect.min, rect.size(), col);
		}

		for(int n = vis_entries.x; n < vis_entries.y; n++) {
			int2 pos = int2(0, m_line_height * n) - offset;
			m_font->drawShadowed(pos + int2(5, 0), m_entries[n].color, Color::black, m_entries[n].text.c_str());
		}
	}

	int2 ListView::visibleEntriesIds() const {
		int2 offset = innerOffset();
		int begin = max(0, offset.y / m_line_height);
		int end   = min((offset.y + rect().height() + m_line_height) / m_line_height, (int)m_entries.size());

		return int2(begin, end);
	}

	int ListView::entryId(int2 pos) const {
		int2 vis_entries = visibleEntriesIds();
		pos += innerOffset();

		for(int n = vis_entries.x; n < vis_entries.y; n++)
			if(entryRect(n).isInside(pos))
				return n;
		return -1;
	}

	void ListView::onInput(int2 mouse_pos) {
		m_over_id = entryId(mouse_pos);
	}

	void ListView::onIdle() {
		m_over_id = -1;
	}

	bool ListView::onMouseDrag(int2 start, int2 end, int key, bool is_final) {
		if(key == 0) {
			m_over_id = entryId(end);
			m_dragging_id = entryId(start);

			if(is_final && m_over_id == m_dragging_id) {
				select(m_over_id);
				sendEvent(this, Event::element_clicked, m_over_id);
				sendEvent(this, Event::element_selected, m_over_id);
			}
			if(is_final)
				m_dragging_id = -1;

			return true;
		}

		return false;
	}

	IRect ListView::entryRect(int entry_id) const {
		int pos = entry_id * m_line_height;
		return IRect(0, pos, rect().width(), pos + m_line_height);
	}

	void ListView::addEntry(const char *text, Color col) {
		m_entries.push_back(Entry{col, text, false});
		update();
	}

	void ListView::clear() {
		m_entries.clear();
		update();
	}

	void ListView::update() {
		setInnerRect(IRect(0, 0, rect().width(), m_font->lineHeight() * (int)m_entries.size()));
	}

	int ListView::selectedId() const {
		for(int n = 0; n < (int)m_entries.size(); n++)
			if(m_entries[n].is_selected)
				return n;
		return -1;
	}

	void ListView::select(int id) {
		for(int n = 0; n < (int)m_entries.size(); n++)
			m_entries[n].is_selected = n == id;
	}

}
