/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef EDITOR_ENTITIES_EDITOR_H
#define EDITOR_ENTITIES_EDITOR_H

#include "ui/window.h"

class TileGroup;

namespace ui {

	class View;

	DECLARE_ENUM(EntitiesEditorMode,
		selecting,
		placing
	);

	class EntitiesEditor: public ui::Window
	{
	public:
		typedef EntitiesEditorMode::Type Mode;

		EntitiesEditor(game::TileMap&, game::EntityMap&, View&, IRect rect);

		void drawContents(Renderer2D&) const override;
		void onInput(const InputState&) override;
		bool onMouseDrag(const InputState&, int2 start, int2 current, int key, int is_final) override;

		Mode mode() const { return m_mode; }
		void setMode(Mode mode) { m_mode = mode; }

		void setProto(game::PEntity);
		void findVisible(vector<int> &out, const IRect &rect) const;
		FBox computeOvergroundBox(const FBox &bbox) const;

	private:
		Mode m_mode;
		View &m_view;
		game::EntityMap &m_entity_map;
		game::TileMap &m_tile_map;
		vector<int> m_selected_ids;

		game::PEntity m_proto;
		int m_proto_angle;
		
		void drawBoxHelpers(Renderer2D&, const IBox &box) const;
		void computeCursor(int2 start, int2 end, bool floor_mode);

		IRect m_selection;
		float3 m_cursor_pos;	
		bool m_is_selecting;

		IBox m_trigger_box;
		int2 m_trigger_offset;
		int m_trigger_mode;

		int3 m_move_offset;
		bool m_is_moving_vertically;
		bool m_is_moving;
	};

	using PEntitiesEditor = shared_ptr<EntitiesEditor>;

}

#endif

