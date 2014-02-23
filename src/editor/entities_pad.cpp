/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "editor/entities_pad.h"
#include "game/base.h"
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

			m_door_id = new ComboBox(second_rect, 200, "Door: ");
			for(int n = 0; n < (int)DoorDesc::count(); n++)
				m_door_id->addEntry(DoorDesc::get(n).id.c_str());
			m_door_id->selectEntry(0);
			
			m_container_id = new ComboBox(second_rect, 200, "Container: ");
			for(int n = 0; n < ContainerDesc::count(); n++)
				m_container_id->addEntry(ContainerDesc::get(n).id.c_str());
			m_container_id->selectEntry(0);

			m_item_type = new ComboBox(second_rect, 200, "Item type: ");
			for(int n = 0; n < game::ItemType::count; n++)
				m_item_type->addEntry(game::ItemType::toString(n));
			m_item_type->selectEntry(0);

			
		}

		{
			IRect third_rect(0, 66, width, 88);

			m_item_id = new ComboBox(third_rect, 200, "Item: ");
			updateItemIds();
		}

		{
			IRect fourth_rect(0, 88, width, 110);

			m_item_count = new EditBox(fourth_rect, 200);
			m_item_count->setText("1");
			m_item_count_val = 1;
		}
		
		attach(m_editor_mode_box.get());
		attach(m_entity_type.get());

		attach(m_actor_type.get());
		attach(m_door_id.get());
		attach(m_container_id.get());
		attach(m_item_type.get());
		attach(m_item_id.get());
		attach(m_item_count.get());

		updateEntity();
		updateVisibility();
	}

	void EntitiesPad::updateItemIds() {
		ItemType::Type type = (ItemType::Type)m_item_type->selectedId();
		DASSERT(ItemType::isValid(type));

		m_item_id->clear();
		for(int n = 0; n < game::Item::count(type); n++) {
			const game::ItemDesc &desc = game::Item::get(game::ItemIndex(n, type));

			if(!desc.is_dummy)
				m_item_id->addEntry(desc.id.c_str());
		}
		m_item_id->selectEntry(0);
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
		else if(ev.type == Event::element_selected && m_item_type.get() == ev.source) {
			updateItemIds();
			updateEntity();
		}
		else if(ev.type == Event::text_modified && m_item_count.get() == ev.source) {
			const char *text = m_item_count->text();
			int tcount = atoi(text);
			tcount = max(1, tcount);
			
			char ttext[64];
			snprintf(ttext, sizeof(ttext), "%d", tcount);
			if(strcmp(ttext, text) != 0)
				m_item_count->setText(ttext);
			m_item_count_val = tcount;
			updateEntity();
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
			int desc_id = m_container_id->selectedId();
			DASSERT(desc_id >= 0 && desc_id < ContainerDesc::count());
			const ContainerDesc &desc = ContainerDesc::get(desc_id);

			m_proto = (PEntity)new game::Container(desc, pos);
		}
		else if(type == EntityId::door) {
			int desc_id = m_door_id->selectedId();
			DASSERT(desc_id >= 0 && desc_id < DoorDesc::count());
			const DoorDesc &desc = DoorDesc::get(desc_id);

			/*
			//TODO: verification of anims?
			PSprite sprite = Sprite::mgr[sprite_name];
			if(!Door::testSpriteType(sprite, type)) {
				for(int n = 0; n < DoorTypeId::count; n++) {
					type = (DoorTypeId::Type)n;
					if(Door::testSpriteType(sprite, type))
						break;
				}
				m_door_type->selectEntry(type);
			}*/

			m_proto = (PEntity)new game::Door(desc, pos);
		}
		else if(type == EntityId::item) {
			ItemType::Type type = (ItemType::Type)m_item_type->selectedId();
			int idx = m_item_id->selectedId();

			m_proto = (PEntity)new game::ItemEntity(game::Item(idx, type), m_item_count_val, pos);
		}

		m_editor->setProto(m_proto.get());
	}

	void EntitiesPad::updateVisibility() {
		EntityId::Type type = s_types[m_entity_type->selectedId()];
		m_actor_type->setVisible(type == EntityId::actor);
		m_door_id->setVisible(type == EntityId::door);
		m_container_id->setVisible(type == EntityId::container);
		m_item_type->setVisible(type == EntityId::item);
		m_item_id->setVisible(type == EntityId::item);
		m_item_count->setVisible(type == EntityId::item);
	}

}
