/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "editor/entities_pad.h"
#include "game/base.h"
#include "game/entity.h"
#include "game/sprite.h"

#include "game/actor.h"
#include "game/turret.h"
#include "game/container.h"
#include "game/door.h"
#include "game/item.h"
#include "game/trigger.h"

#include <algorithm>

using namespace game;

namespace ui {

	EntityPad::EntityPad(const IRect &max_rect, EntityId type_id)
		:Window(IRect(max_rect.min(), int2(max_rect.ex(), max_rect.y() + 1))), m_max_rect(max_rect), m_type_id(type_id) { }

	void EntityPad::addControl(PWindow window) {
		DASSERT(window);
		IRect pad_rect = rect();
		IRect crect = window->rect();
		pad_rect = {pad_rect.min(), {pad_rect.ex(),
			std::min(std::max(pad_rect.ey(), pad_rect.y() + crect.ey()), m_max_rect.ey())}};
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



	TurretPad::TurretPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::turret) {
		m_proto_id = addControl<ComboBox>(200, "Turret: ");
		for(int n = 0; n < countProtos(ProtoId::turret); n++)
			m_proto_id->addEntry(getProto(n, ProtoId::turret).id.c_str());
		m_proto_id->selectEntry(0);
	}
		
	PEntity TurretPad::makeEntity() const {
		const Proto& proto = getProto(m_proto_id->selectedText(), ProtoId::turret);
		return make_unique<Turret>(proto);
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
		return make_unique<Door>(proto);
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
		return make_unique<Container>(proto);
	}



	ItemPad::ItemPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::item) {
		m_type_id = addControl<ComboBox>(200, "Item type: ");
		for(auto item : all<ItemType>())
			m_type_id->addEntry(toString(item));
		m_type_id->selectEntry(0);
		m_proto_id = addControl<ComboBox>(200, "Item id: ");

		m_count = addControl<EditBox>(20, "Count: ");
		m_count->setText(L"1");
		m_count_val = 1;

		updateItemIds();
	}
		
	PEntity ItemPad::makeEntity() const {
		ItemType type = (ItemType)m_type_id->selectedId();
		ProtoId proto_id = toProtoId(type);
		ProtoIndex index = findProto((*m_proto_id)[m_proto_id->selectedId()].text, proto_id);
		return make_unique<ItemEntity>(Item(index), m_count_val);
	}

	void ItemPad::updateItemIds() {
		ItemType type = (ItemType)m_type_id->selectedId();
		DASSERT(validEnum(type));
		ProtoId proto_id = toProtoId(type);

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
			auto text = fromWideString(m_count->text());
			int tcount = max(1, fromString<int>(text));
			string new_text = toString(tcount);
			if(text != new_text)
				m_count->setText(toWideString(new_text));
			m_count_val = tcount;
		}

		return false;
	}



	TriggerPad::TriggerPad(const IRect &max_rect) :EntityPad(max_rect, EntityId::trigger) {
		m_class_id = addControl<ComboBox>(200, "Trigger class: ");
		for(auto tcid : all<TriggerClassId>())
			m_class_id->addEntry(toString(tcid));
		m_class_id->selectEntry(0);
		m_faction_id = addControl<EditBox>(200, "Faction id: ");
		m_faction_id_val = 0;
		m_faction_id->setText(L"0");
	}
	
	bool TriggerPad::onEvent(const Event &ev) {
		if(ev.type == Event::text_modified && m_faction_id == ev.source) {
			auto text = fromWideString(m_faction_id->text());
			int tcount = max(0, fromString<int>(text));
			string new_text = toString(tcount);
			if(text != new_text)
				m_faction_id->setText(toWideString(new_text));
			m_faction_id_val = tcount;
		}

		return false;
	}	
	PEntity TriggerPad::makeEntity() const {
		auto out = make_unique<Trigger>((TriggerClassId)m_class_id->selectedId(), FBox(0, 0, 0, 1, 1, 1));
		out->setFactionId(m_faction_id_val);
		return (PEntity)(out.release());
	}

	EntitiesPad::EntitiesPad(const IRect &rect, PEntitiesEditor editor)
		:Window(rect, ColorId::transparent), m_editor(editor) {
		int width = rect.width();

		m_editor_mode_box = make_shared<ComboBox>(IRect(0, 0, width, WindowStyle::line_height), 200, "Editing mode: ");
		for(auto mode : all<Mode>())
			m_editor_mode_box->addEntry(describe(mode));
		m_editor_mode_box->selectEntry((int)m_editor->mode());


		m_entity_type = make_shared<ComboBox>(IRect(0, WindowStyle::line_height, width, WindowStyle::line_height * 2), 200, "Entity type: ");
		
		IRect pad_rect(0, WindowStyle::line_height * 2, width, WindowStyle::line_height * 12);
		m_pads.emplace_back(make_shared<ActorPad>(pad_rect));
		m_pads.emplace_back(make_shared<TurretPad>(pad_rect));
		m_pads.emplace_back(make_shared<ContainerPad>(pad_rect));
		m_pads.emplace_back(make_shared<DoorPad>(pad_rect));
		m_pads.emplace_back(make_shared<ItemPad>(pad_rect));
		m_pads.emplace_back(make_shared<TriggerPad>(pad_rect));

		attach(m_editor_mode_box);
		attach(m_entity_type);

		for(int n = 0; n < (int)m_pads.size(); n++) {
			attach(m_pads[n]);
			m_entity_type->addEntry(toString(m_pads[n]->typeId()));
		}
		m_entity_type->selectEntry(0);

		updateEntity();
		updateVisibility();
	}

	bool EntitiesPad::onEvent(const Event &ev) {
		if(ev.type == Event::button_clicked && m_editor.get() == ev.source) {
			if(m_editor_mode_box->selectedId() != (int)m_editor->mode())
				m_editor_mode_box->selectEntry((int)m_editor->mode());
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

	EntityId EntitiesPad::selectedTypeId() const {
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
