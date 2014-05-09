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
#include "game/trigger.h"

#include "sys/platform.h"
#include <algorithm>

using namespace game;

namespace ui {

	EntityPad::EntityPad(const IRect &max_rect, EntityId::Type type_id)
		:Window(IRect(max_rect.min, int2(max_rect.max.x, max_rect.min.y + 1))), m_max_rect(max_rect), m_type_id(type_id) { }

	void EntityPad::addControl(PWindow window) {
		DASSERT(window);
		IRect pad_rect = rect();
		IRect crect = window->rect();
		pad_rect.max.y = std::min(std::max(pad_rect.max.y, pad_rect.min.y + crect.max.y), m_max_rect.max.y);
		if(pad_rect != rect())
			setRect(pad_rect);
		attach(window);
	}
	


	ActorPad::ActorPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::actor) {
		m_proto_id = addControl<ComboBox>(200, "Actor: ");
		for(int n = 0; n < countProtos(ProtoId::actor); n++)
			m_proto_id->addEntry(getProto(n, ProtoId::actor).id.c_str());
		m_proto_id->selectEntry(0);
	}
		
	PEntity ActorPad::makeEntity() const {
		const Proto& proto = getProto(m_proto_id->selectedText(), ProtoId::actor);
		return (PEntity)new Actor(proto);
	}



	DoorPad::DoorPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::door) {
		m_proto_id = addControl<ComboBox>(200, "Door: ");
		for(int n = 0; n < countProtos(ProtoId::door); n++)
			m_proto_id->addEntry(getProto(n, ProtoId::door).id.c_str());
		m_proto_id->selectEntry(0);
	}
		
	PEntity DoorPad::makeEntity() const {
		const DoorProto &proto =
			static_cast<const DoorProto&>(getProto(m_proto_id->selectedId(), ProtoId::door));
		return (PEntity)new Door(proto);
	}



	ContainerPad::ContainerPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::container) {
		m_proto_id = addControl<ComboBox>(200, "Container: ");
		for(int n = 0; n < countProtos(ProtoId::container); n++)
			m_proto_id->addEntry(getProto(n, ProtoId::container).id.c_str());
		m_proto_id->selectEntry(0);
	}
		
	PEntity ContainerPad::makeEntity() const {
		const ContainerProto &proto =
			static_cast<const ContainerProto&>(getProto(m_proto_id->selectedId(), ProtoId::container));
		return (PEntity)new Container(proto);
	}



	ItemPad::ItemPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::item) {
		m_type_id = addControl<ComboBox>(200, "Item type: ");
		for(int n = 0; n < ItemType::count; n++)
			m_type_id->addEntry(ItemType::toString(n));
		m_type_id->selectEntry(0);
		m_proto_id = addControl<ComboBox>(200, "Item id: ");

		m_count = addControl<EditBox>(20, "Count: ");
		m_count->setText("1");
		m_count_val = 1;

		updateItemIds();
	}
		
	PEntity ItemPad::makeEntity() const {
		ItemType::Type type = (ItemType::Type)m_type_id->selectedId();
		ProtoId::Type proto_id = ItemType::toProtoId(type);
		ProtoIndex index = findProto((*m_proto_id)[m_proto_id->selectedId()].text, proto_id);
		return (PEntity)new ItemEntity(Item(index), m_count_val);
	}

	void ItemPad::updateItemIds() {
		ItemType::Type type = (ItemType::Type)m_type_id->selectedId();
		DASSERT(ItemType::isValid(type));
		ProtoId::Type proto_id = ItemType::toProtoId(type);

		m_proto_id->clear();
		for(int n = 0; n < countProtos(proto_id); n++) {
			const game::ItemProto &proto =
				static_cast<const game::ItemProto&>(getProto(n, proto_id));
			if(!proto.is_dummy)
				m_proto_id->addEntry(proto.id.c_str());
		}
		m_proto_id->selectEntry(0);
	}


	bool ItemPad::onEvent(const Event &ev) {
		if(ev.type == Event::element_selected && m_type_id.get() == ev.source) {
			updateItemIds();
		}
		else if(ev.type == Event::text_modified && m_count.get() == ev.source) {
			const char *text = m_count->text();
			int tcount = atoi(text);
			tcount = max(1, tcount);
			
			char ttext[64];
			snprintf(ttext, sizeof(ttext), "%d", tcount);
			if(strcmp(ttext, text) != 0)
				m_count->setText(ttext);
			m_count_val = tcount;
		}

		return false;
	}



	TriggerPad::TriggerPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::trigger) {
		m_class_id = addControl<ComboBox>(200, "Trigger class: ");
		for(int n = 0; n < TriggerClassId::count; n++)
			m_class_id->addEntry(TriggerClassId::toString(n));
		m_class_id->selectEntry(0);
	}
		
	PEntity TriggerPad::makeEntity() const {
		return (PEntity)new Trigger((TriggerClassId::Type)m_class_id->selectedId(), FBox(0, 0, 0, 1, 1, 1));
	}



	EntitiesPad::EntitiesPad(const IRect &rect, PEntitiesEditor editor)
		:Window(rect, Color::transparent), m_editor(editor) {
		int width = rect.width();

		m_editor_mode_box = new ComboBox(IRect(0, 0, width, WindowStyle::line_height), 200, "Editing mode: ");
		for(int n = 0; n < Mode::count; n++)
			m_editor_mode_box->addEntry(EntitiesEditorMode::toString(n));
		m_editor_mode_box->selectEntry(m_editor->mode());


		m_entity_type = new ComboBox(IRect(0, WindowStyle::line_height, width, WindowStyle::line_height * 2), 200, "Entity type: ");
		
		IRect pad_rect(0, WindowStyle::line_height * 2, width, WindowStyle::line_height * 12);
		m_pads.emplace_back((PEntityPad)new ActorPad(pad_rect));
		m_pads.emplace_back((PEntityPad)new ContainerPad(pad_rect));
		m_pads.emplace_back((PEntityPad)new DoorPad(pad_rect));
		m_pads.emplace_back((PEntityPad)new ItemPad(pad_rect));
		m_pads.emplace_back((PEntityPad)new TriggerPad(pad_rect));

		attach(m_editor_mode_box.get());
		attach(m_entity_type.get());

		for(int n = 0; n < (int)m_pads.size(); n++) {
			attach((PWindow)m_pads[n].get());
			m_entity_type->addEntry(EntityId::toString(m_pads[n]->typeId()));
		}
		m_entity_type->selectEntry(0);

		updateEntity();
		updateVisibility();
	}

	bool EntitiesPad::onEvent(const Event &ev) {
		if(ev.type == Event::button_clicked && m_editor.get() == ev.source) {
			if(m_editor_mode_box->selectedId() != m_editor->mode())
				m_editor_mode_box->selectEntry(m_editor->mode());
		}
		else if(ev.type == Event::element_selected && m_editor_mode_box.get() == ev.source)
			m_editor->setMode((EntitiesEditor::Mode)ev.value);
		else if(ev.type == Event::element_selected && m_entity_type.get() == ev.source) {
			updateVisibility();
			updateEntity();
		}
		else if(ev.source && ev.source->parent() && ev.source->parent()->parent() == this) {
			updateEntity();
		}
		else
			return false;

		return true;
	}

	EntityId::Type EntitiesPad::selectedTypeId() const {
		return m_pads[m_entity_type->selectedId()]->typeId();
	}

	PEntity EntitiesPad::makeEntity() const {
		return m_pads[m_entity_type->selectedId()]->makeEntity();
	}
		
	void EntitiesPad::updateEntity() {
		m_editor->setProto(makeEntity());
	}

	void EntitiesPad::updateVisibility() {
		int selected_id = m_entity_type->selectedId();
		for(int n = 0; n < (int)m_pads.size(); n++)
			m_pads[n]->setVisible(n == selected_id);
	}

}
