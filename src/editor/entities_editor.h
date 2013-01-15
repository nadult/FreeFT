#ifndef EDITOR_ENTITIES_EDITOR_H
#define EDITOR_ENTITIES_EDITOR_H

#include "ui/window.h"

class TileGroup;

namespace ui {

	class EntitiesEditor: public ui::Window
	{
	public:
		EntitiesEditor(IRect rect);

		void setEntityMap(game::EntityMap*);
		void setTileMap(game::TileMap*);
			
		virtual void drawContents() const;
		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 current, int key, int is_final);

		enum Mode {
			mode_selecting,
			mode_placing,

			mode_count,
		} m_mode;

	private:
		game::EntityMap *m_entity_map;
		game::TileMap *m_tile_map;
		vector<int> m_selected_ids;
		
		void drawBoxHelpers(const IBox &box) const;
		IBox computeCursor(int2 start, int2 end) const;
		void clampViewPos();
		
		int m_cursor_height;
		IBox m_selection;
		int2 m_view_pos;
		bool m_is_selecting;
	};

	typedef Ptr<EntitiesEditor> PEntitiesEditor;

}


#endif

