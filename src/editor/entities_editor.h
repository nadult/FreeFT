/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef EDITOR_ENTITIES_EDITOR_H
#define EDITOR_ENTITIES_EDITOR_H

#include "ui/window.h"

class TileGroup;

namespace ui {

	class View;

	class EntitiesEditor: public ui::Window
	{
	public:
		EntitiesEditor(game::TileMap&, game::EntityMap&, View&, IRect rect);

		virtual void drawContents() const;
		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 current, int key, int is_final);

		enum Mode {
			mode_selecting,
			mode_placing,

			mode_count,
		};

		Mode mode() const { return m_mode; }
		void setMode(Mode mode) { m_mode = mode; }
		
		static const char **modeStrings();

		void setProto(game::Entity *proto);

	private:
		Mode m_mode;
		View &m_view;
		game::EntityMap &m_entity_map;
		game::TileMap &m_tile_map;
		vector<int> m_selected_ids;

		game::Entity *m_proto;
		int m_proto_angle;
		
		void drawBoxHelpers(const IBox &box) const;
		void computeCursor(int2 start, int2 end);
	
		IRect m_selection;
		float3 m_cursor_pos;	
		bool m_is_selecting;
	};

	typedef Ptr<EntitiesEditor> PEntitiesEditor;

}


#endif

