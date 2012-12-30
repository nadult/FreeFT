#ifndef UI_COMBO_BOX_H
#define UI_COMBO_BOX_H

#include "ui/window.h"
#include "ui/button.h"
#include "ui/list_box.h"
#include "gfx/font.h"

namespace ui {

	class ComboBox: public Window {
	public:
		ComboBox(const IRect &rect, int drop_size, const char *prefix = "",
					const char **values = nullptr, int value_count = 0);
	
		virtual bool onEvent(const Event &ev);

		void addEntry(const char *text, Color col);
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
