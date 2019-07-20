// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "editor/group_editor.h"
#include "ui/combo_box.h"

namespace ui {

	class GroupPad: public Window
	{
	public:
		GroupPad(const IRect &rect, PGroupEditor editor, TileGroup *group);
		TileFilter currentFilter() const;
		bool onEvent(const Event&) override;

		PComboBox		m_filter_box;
		TileGroup		*m_group;
		PGroupEditor	m_editor;
	};

	using PGroupPad = shared_ptr<GroupPad>;

}
