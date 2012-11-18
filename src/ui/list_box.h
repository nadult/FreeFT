#ifndef UI_LIST_BOX_H
#define UI_LIST_BOX_H

#include "ui/window.h"
#include "gfx/font.h"

namespace ui {

	class ListBox: public ui::Window
	{
	public:
		ListBox(const IRect &rect);
		virtual const char *typeName() const { return "ListBox"; }

		struct Entry {
			Color color;
			string text;
			mutable bool is_selected;
		};
		
		virtual void drawContents() const;
		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 end, int key, int is_final);

		void addEntry(const char *text, Color col);
		int findEntry(const char*) const;
		int selectedId() const;
		void selectEntry(int id);
		void clear();

		int m_max_width, m_spacing;
		int m_height;

		int size() const { return (int)m_entries.size(); }
		const Entry& operator[](int idx) const { return m_entries[idx]; }
		Entry& operator[](int idx) { return m_entries[idx]; }

	private:
		void update();
		IRect entryRect(int id) const;
		int2 visibleEntriesIds() const;
		int entryId(int2 pos) const;

		vector<Entry> m_entries;
		gfx::PFont m_font;
		int m_line_height;
		int m_over_id, m_dragging_id;
	};

	typedef Ptr<ListBox> PListBox;


}


#endif
