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

