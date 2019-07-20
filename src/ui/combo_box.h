// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "ui/window.h"
#include "ui/button.h"
#include "ui/list_box.h"

namespace ui {

	// if drop_size == 0 then drop list won't be shown,
	// items will be selected in cyclic order
	class ComboBox: public Window {
	public:
		ComboBox(const IRect &rect, int drop_size, const char *prefix = "",
					CSpan<const char *> values = {});
	
		bool onEvent(const Event &ev) override;

		void addEntry(const char *text, Color col = ColorId::white);
		int findEntry(const char*) const;
		int selectedId() const;
		void selectEntry(int id);
		void clear();
		int size() const;
		
		const string &selectedText() const {
			DASSERT(size() > 0);
			return (*this)[selectedId()].text;
		}

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

	using PComboBox = shared_ptr<ComboBox>;

}
