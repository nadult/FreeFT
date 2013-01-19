/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.

   FreeFT is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FreeFT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "ui/combo_box.h"

namespace ui {

	ComboBox::ComboBox(const IRect &rect, int drop_size, const char *prefix, const char **values, int value_count)
		:Window(rect), m_drop_size(drop_size), m_prefix(prefix) {
		m_button = new Button(IRect(int2(0, 0), rect.size()), "");
		m_dummy = new ListBox(IRect(0, 0, 10, 10));
		attach(m_button.get());

		if(values) {
			for(int n = 0; n < value_count; n++)
				addEntry(values[n], Color::white);
			if(value_count)
				selectEntry(0);
		}
	}

	bool ComboBox::onEvent(const Event &ev) {
		if(ev.type == Event::window_closed && ev.source == m_popup.get()) {
			m_dummy->selectEntry(m_popup->selectedId());
			if(ev.value)
				sendEvent(this, Event::element_selected, m_dummy->selectedId());
			m_popup = nullptr;
			updateButton();
		}
		else if(ev.type == Event::button_clicked && ev.source == m_button.get()) {
			DASSERT(!m_popup);

			if(m_drop_size > 0) {
				int popup_size = min(m_drop_size, m_dummy->size() * m_dummy->lineHeight());
				IRect clip_rect = clippedRect();
				IRect rect(clip_rect.min, int2(clip_rect.max.x, clip_rect.min.y + popup_size));

				m_popup = new ListBox(rect, Color::gui_popup);
				for(int n = 0; n < size(); n++) {
					const ListBox::Entry &entry = (*m_dummy)[n];
					m_popup->addEntry(entry.text.c_str(), entry.color);
				}
				m_popup->selectEntry(m_dummy->selectedId());
				mainWindow()->attach(m_popup.get(), true);
			}
			else if(size() > 1) {
				int next_id = (selectedId() + 1) % size();
				selectEntry(next_id);
				sendEvent(this, Event::element_selected, next_id);
			}
		}
		else
			return false;

		return true;
	}

	void ComboBox::addEntry(const char *text, Color col) {
		DASSERT(!m_popup);
		m_dummy->addEntry(text, col);
		updateButton();
	}

	int ComboBox::findEntry(const char *text) const {
		return m_dummy->findEntry(text);
	}

	int ComboBox::selectedId() const {
		return (m_popup? m_popup : m_dummy)->selectedId();
	}

	void ComboBox::selectEntry(int id) {
		(m_popup? m_popup : m_dummy)->selectEntry(id);
		updateButton();
	}

	void ComboBox::clear() {
		DASSERT(!m_popup);
		m_dummy->clear();
		updateButton();
	}
		
	const ListBox::Entry& ComboBox::operator[](int idx) const {
		return m_dummy->operator[](idx);
	}

	ListBox::Entry& ComboBox::operator[](int idx) {
		DASSERT(!m_popup);
		return m_dummy->operator[](idx);
	}

	int ComboBox::size() const {
		return m_dummy->size();
	}

	void ComboBox::updateButton() {
		int selected_id = m_dummy->selectedId();
		char buf[256];
		snprintf(buf, sizeof(buf), "%s%s", m_prefix.c_str(),
				selected_id >= 0 && selected_id < m_dummy->size()? (*m_dummy)[selected_id].text.c_str() : "");
		m_button->setText(buf);
	}

}
