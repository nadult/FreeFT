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

#include "editor/entities_pad.h"
#include "game/enums.h"
#include "game/entity.h"
#include "game/sprite.h"

#include "game/actor.h"
#include "game/container.h"
#include "game/door.h"
#include "game/item.h"

#include "sys/platform.h"
#include <algorithm>

using namespace game;

namespace ui {

	EntityId::Type s_types[4] = {
		EntityId::actor,
		EntityId::container,
		EntityId::door,
		EntityId::item,
	};

	EntitiesPad::EntitiesPad(const IRect &rect, PEntitiesEditor editor)
		:Window(rect, Color::transparent), m_editor(editor) {
		int width = rect.width();

		m_editor_mode_box = new ComboBox(IRect(0, 0, width, 22), 200, "Editing mode: ",
				EntitiesEditor::modeStrings(), EntitiesEditor::mode_count);
		m_entity_type = new ComboBox(IRect(0, 22, width, 44), 200, "Entity type: ");
		for(int n = 0; n < COUNTOF(s_types); n++)
			m_entity_type->addEntry(EntityId::toString(s_types[n]));
		m_entity_type->selectEntry(0);

		{
			IRect second_rect(0, 44, width, 66);

			m_actor_type = new ComboBox(second_rect, 200, "Actor type: ");
			for(int n = 0; n < ActorTypeId::count; n++)
				m_actor_type->addEntry(ActorTypeId::toString((ActorTypeId::Type)n));
			m_actor_type->selectEntry(0);

			findSprites(m_door_sprite_names, "doors");
			findSprites(m_container_sprite_names, "containers");

			m_door_sprite = new ComboBox(second_rect, 200, "Door sprite: ");
			for(int n = 0; n < (int)m_door_sprite_names.size(); n++)
				m_door_sprite->addEntry(m_door_sprite_names[n].c_str() + 6);
			m_door_sprite->selectEntry(0);
			
			m_container_sprite = new ComboBox(second_rect, 200, "Container sprite: ");
			for(int n = 0; n < (int)m_container_sprite_names.size(); n++)
				m_container_sprite->addEntry(m_container_sprite_names[n].c_str() + 11);
			m_container_sprite->selectEntry(0);

			m_item_type = new ComboBox(second_rect, 200, "Item: ");
			for(int n = 0; n < game::ItemDesc::count(); n++)
				m_item_type->addEntry(game::ItemDesc::get(n)->name.c_str());
			m_item_type->selectEntry(0);
		}

		{
			IRect third_rect(0, 66, width, 88);

			m_door_type = new ComboBox(third_rect, 200, "Door type: ");
			for(int n = 0; n < DoorTypeId::count; n++)
				m_door_type->addEntry(DoorTypeId::toString((DoorTypeId::Type)n));
			m_door_type->selectEntry(0);
		}
		
		attach(m_editor_mode_box.get());
		attach(m_entity_type.get());

		attach(m_actor_type.get());
		attach(m_door_sprite.get());
		attach(m_container_sprite.get());
		attach(m_item_type.get());
		
		attach(m_door_type.get());

		updateEntity();
		updateVisibility();
	}

	void EntitiesPad::findSprites(vector<string> &out, const char *path) {
		vector<FileEntry> entries;
		findFiles(entries, Sprite::mgr.prefix() + "/" + path, FindFiles::regular_file|FindFiles::recursive);
		for(int n = 0; n < (int)entries.size(); n++) {
			string name = entries[n].path;
			if(removePrefix(name, Sprite::mgr.prefix()) && removeSuffix(name, Sprite::mgr.suffix()))
				out.push_back(name);
		}
		std::sort(out.begin(), out.end());
	}
	
	bool EntitiesPad::onEvent(const Event &ev) {
		if(ev.type == Event::button_clicked && m_editor.get() == ev.source) {
			if(m_editor_mode_box->selectedId() != m_editor->mode())
				m_editor_mode_box->selectEntry(m_editor->mode());
		}
		else if(ev.type == Event::element_selected && m_editor_mode_box.get() == ev.source)
			m_editor->setMode((EntitiesEditor::Mode)ev.value);
		else if(ev.type == Event::element_selected && m_entity_type.get() == ev.source) {
			updateEntity();
			updateVisibility();
		}
		else if(ev.source && ev.source->parent() == this) {
			updateEntity();
		}
		else
			return false;

		return true;
	}

	void EntitiesPad::updateEntity() {
		EntityId::Type type = s_types[m_entity_type->selectedId()];
		m_editor->setProto(nullptr);
		m_proto = nullptr;

		float3 pos(0, 0, 0);
		
		if(type == EntityId::actor)
			m_proto = (PEntity)new game::Actor((ActorTypeId::Type)m_actor_type->selectedId(), pos);
		else if(type == EntityId::container) {
			const char *sprite_name = m_container_sprite_names[m_container_sprite->selectedId()].c_str();
			m_proto = (PEntity)new game::Container(sprite_name, pos);
		}
		else if(type == EntityId::door) {
			const char *sprite_name = m_door_sprite_names[m_door_sprite->selectedId()].c_str();
			DoorTypeId::Type type = (DoorTypeId::Type)m_door_type->selectedId();
			PSprite sprite = Sprite::mgr[sprite_name];
			if(!Door::testSpriteType(sprite, type)) {
				for(int n = 0; n < DoorTypeId::count; n++) {
					type = (DoorTypeId::Type)n;
					if(Door::testSpriteType(sprite, type))
						break;
				}
				m_door_type->selectEntry(type);
			}

			m_proto = (PEntity)new game::Door(sprite_name, pos, type);
		}
		else if(type == EntityId::item) {
			m_proto = (PEntity)new game::ItemEntity(game::ItemDesc::get(m_item_type->selectedId()), pos);
		}

		m_editor->setProto(m_proto.get());
	}

	void EntitiesPad::updateVisibility() {
		EntityId::Type type = s_types[m_entity_type->selectedId()];
		m_actor_type->setVisible(type == EntityId::actor);
		m_door_sprite->setVisible(type == EntityId::door);
		m_door_type->setVisible(type == EntityId::door);
		m_container_sprite->setVisible(type == EntityId::container);
		m_item_type->setVisible(type == EntityId::item);
	}

}
