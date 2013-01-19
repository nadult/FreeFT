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

#ifndef UI_COMBO_BOX_H
#define UI_COMBO_BOX_H

#include "ui/window.h"
#include "ui/button.h"
#include "ui/list_box.h"
#include "gfx/font.h"

namespace ui {

	// if drop_size == 0 then drop list won't be shown,
	// items will be selected in cyclic order
	class ComboBox: public Window {
	public:
		ComboBox(const IRect &rect, int drop_size, const char *prefix = "",
					const char **values = nullptr, int value_count = 0);
	
		virtual bool onEvent(const Event &ev);

		void addEntry(const char *text, Color col = Color::white);
		int findEntry(const char*) const;
		int selectedId() const;
		void selectEntry(int id);
		void clear();
		int size() const;
		
		const ListBox::Entry& operator[](int idx) const;
		ListBox::Entry& operator[](int idx);
		void updateButton();

	protected:
		string m_prefix;
		PButton m_button;
		PListBox m_dummy;
		PListBox m_popup;
		int m_drop_size;
	};

	typedef Ptr<ComboBox> PComboBox;

}

#endif
