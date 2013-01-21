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

#ifndef EDITOR_ENTITIES_PAD_H
#define EDITOR_ENTITIES_PAD_H

#include "editor/entities_editor.h"
#include "ui/combo_box.h"
#include "ui/progress_bar.h"

namespace ui {

	class EntitiesPad: public Window
	{
	public:
		EntitiesPad(const IRect &rect, PEntitiesEditor editor);
		virtual bool onEvent(const Event &ev);

	protected:
		void updateEntity();
		void updateVisibility();
		void findSprites(vector<string> &out, const char *path);

		PEntitiesEditor	m_editor;
		PComboBox		m_editor_mode_box;

		PComboBox		m_entity_type;

		PComboBox		m_actor_type;
		PComboBox		m_door_sprite;
		PComboBox		m_container_sprite;
		PComboBox		m_item_type;

		PComboBox		m_door_type;

		game::PEntity	m_proto;

		vector<string>	m_door_sprite_names;
		vector<string>	m_container_sprite_names;
	};
	
	typedef Ptr<EntitiesPad> PEntitiesPad;

}

#endif
