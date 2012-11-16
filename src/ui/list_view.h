#ifndef UI_LIST_VIEW_H
#define UI_LIST_VIEW_H

#include "ui/window.h"
#include "gfx/font.h"

namespace ui {

	class ListView: public ui::Window
	{
	public:
		ListView(const IRect &rect);
		virtual const char *typeName() const { return "ListView"; }

		struct Entry {
			Color color;
			string text;
			mutable bool is_selected;
		};
		
		virtual void drawContents() const;
		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 end, int key, bool is_final);

		void addEntry(const char *text, Color col);
		void clear();
		int selectedId() const;
		void select(int id);

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

	typedef Ptr<ListView> PListView;


}


#endif
