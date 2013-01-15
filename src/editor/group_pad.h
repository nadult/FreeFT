/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

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
