/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef EDITOR_ENTITIES_PAD_H
#define EDITOR_ENTITIES_PAD_H

#include "editor/entities_editor.h"
#include "ui/combo_box.h"
#include "ui/edit_box.h"
#include "ui/progress_bar.h"

namespace ui {

	class EntitiesPad: public Window
	{
	public:
		EntitiesPad(const IRect &rect, PEntitiesEditor editor);
		virtual bool onEvent(const Event &ev);

	protected:
		void updateItemIds();
		void updateEntity();
		void updateVisibility();

		PEntitiesEditor	m_editor;
		PComboBox		m_editor_mode_box;

		PComboBox		m_entity_type;

		PComboBox		m_actor_type;
		PComboBox		m_door_id;
		PComboBox		m_container_id;

		PComboBox		m_item_type;
		PComboBox		m_item_id;
		PEditBox		m_item_count;
		int				m_item_count_val;

		game::PEntity	m_proto;
	};
	
	typedef Ptr<EntitiesPad> PEntitiesPad;

}

#endif
