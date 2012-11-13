#ifndef UI_TILE_GROUP_EDITOR_H
#define UI_TILE_GROUP_EDITOR_H

#include "base.h"
#include "gfx/tile.h"
#include "gfx/font.h"
#include "gfx/device.h"
#include "ui/window.h"
#include "ui/tile_list.h"

class TileGroup;

namespace ui {

	class TileGroupEditor: public ui::Window {
	public:
		TileGroupEditor(IRect rect);

		virtual void onInput(int2 mouse_pos);
		virtual bool onMouseDrag(int2 start, int2 current, int key, bool is_final);
		virtual void drawContents() const;

		void setTarget(TileGroup* tile_group);

	protected:
		void updateSelector();

		ui::TileList m_tile_list;
		TileGroup *m_tile_group;

		gfx::PFont m_font;
		IRect m_view;

		int2 m_offset[3];
		int m_selected_group_id;
		int m_selected_surface_id;
		int m_select_mode;
		int m_selection_mode;
		enum {
			mAddRemove, // add / remove tiles to / from group
			mModify, // modify tiles relations
		} m_mode;
	};

	typedef Ptr<TileGroupEditor> PTileGroupEditor;

}


#endif
