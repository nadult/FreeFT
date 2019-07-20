// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "editor/entities_editor.h"
#include "ui/combo_box.h"
#include "ui/edit_box.h"
#include "ui/progress_bar.h"
#include "game/entity.h"
		
namespace ui {

	using EntityId = game::EntityId;
	using game::PEntity;

	class EntityPad: public Window {
	public:
		EntityPad(const IRect &max_rect, EntityId);
		virtual ~EntityPad() = default;

		template <class TControl, class ...Args>
		shared_ptr<TControl> addControl(const Args&... args) {
			IRect pad_rect = rect();
			IRect crect(0, pad_rect.height(), pad_rect.width(), pad_rect.height() + WindowStyle::line_height);
			auto control = make_shared<TControl>(crect, args...);
			addControl((PWindow)control);
			return control;
		}

		EntityId typeId() const { return m_type_id; }

		virtual PEntity makeEntity() const = 0;

	private:
		void addControl(PWindow);

		EntityId m_type_id;
		IRect m_max_rect;
	};

	using PEntityPad = shared_ptr<EntityPad>;

	class ActorPad: public EntityPad {
	public:
		ActorPad(const IRect &max_rect);
		PEntity makeEntity() const override;

	protected:
		PComboBox m_proto_id;
	};

	class TurretPad: public EntityPad {
	public:
		TurretPad(const IRect &max_rect);
		PEntity makeEntity() const override;

	protected:
		PComboBox m_proto_id;
	};


	class DoorPad: public EntityPad {
	public:
		DoorPad(const IRect &max_rect);
		PEntity makeEntity() const override;

	protected:
		PComboBox m_proto_id;
	};

	class ContainerPad: public EntityPad {
	public:
		ContainerPad(const IRect &max_rect);
		PEntity makeEntity() const override;

	protected:
		PComboBox m_proto_id;
	};

	class ItemPad: public EntityPad {
	public:
		ItemPad(const IRect &max_rect);
		PEntity makeEntity() const override;
		bool onEvent(const Event &ev) override;

	protected:
		void updateItemIds();

		PComboBox m_type_id;
		PComboBox m_proto_id;
		PEditBox m_count;
		int m_count_val;
	};

	class TriggerPad: public EntityPad {
	public:
		TriggerPad(const IRect &max_rect);
		PEntity makeEntity() const override;
		bool onEvent(const Event &ev) override;

	protected:
		PComboBox m_class_id;
		PEditBox m_faction_id;
		int m_faction_id_val;
	};

	class EntitiesPad: public Window
	{
	public:
		EntitiesPad(const IRect &rect, PEntitiesEditor editor);
		bool onEvent(const Event &ev) override;
		
		EntityId selectedTypeId() const;
		PEntity makeEntity() const;

	protected:
		void updateEntity();
		void updateVisibility();

		typedef EntitiesEditorMode Mode;

		PEntitiesEditor	m_editor;
		PComboBox m_editor_mode_box;
		PComboBox m_entity_type;
		vector<PEntityPad> m_pads;
	};
	
	using PEntitiesPad = shared_ptr<EntitiesPad>;

}
