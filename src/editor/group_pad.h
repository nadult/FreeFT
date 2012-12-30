#ifndef EDITOR_GROUP_PAD_H
#define EDITOR_GROUP_PAD_H

#include "editor/group_editor.h"
#include "ui/combo_box.h"

namespace ui {

	class GroupPad: public Window
	{
	public:
		GroupPad(const IRect &rect, PGroupEditor editor, TileGroup *group);
		TileFilter::Type currentFilter() const;
		virtual bool onEvent(const Event &ev);

		PComboBox		m_filter_box;
		TileGroup		*m_group;
		PGroupEditor	m_editor;
	};

	typedef Ptr<GroupPad> PGroupPad;

}

#endif
