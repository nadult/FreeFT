// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "ui/window.h"

namespace ui {

	// When created as a popup, it behaves a bit differently,
	// so it can be used in a combo box
	class ListBox: public ui::Window
	{
	public:
		ListBox(const IRect &rect, FColor color = WindowStyle::gui_dark);
		const char *typeName() const override { return "ListBox"; }

		struct Entry {
			FColor color;
			string text;
			mutable bool is_selected;
		};
		
		bool onEvent(const Event&) override;
		void drawContents(Renderer2D&) const override;
		void onInput(const InputState&) override;
		bool onMouseDrag(const InputState&, int2 start, int2 end, int key, int is_final) override;

		void addEntry(const char *text, FColor col = ColorId::white);
		int findEntry(const char*) const;
		int selectedId() const;
		void selectEntry(int id);
		void clear();

		int size() const { return (int)m_entries.size(); }
		const Entry& operator[](int idx) const { return m_entries[idx]; }
		Entry& operator[](int idx) { return m_entries[idx]; }

		int lineHeight() const { return m_line_height; }

	private:
		void update();
		IRect entryRect(int id) const;
		int2 visibleEntriesIds() const;
		int entryId(int2 pos) const;

		vector<Entry> m_entries;
		const Font &m_font;
		int m_line_height;
		int m_over_id, m_dragging_id;
	};

	using PListBox = shared_ptr<ListBox>;


}
