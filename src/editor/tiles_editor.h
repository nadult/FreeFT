/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef EDITOR_TILES_EDITOR_H
#define EDITOR_TILES_EDITOR_H

#include "ui/window.h"

class TileGroup;

namespace ui {

	class View;

	class TilesEditor: public ui::Window
	{
	public:
		TilesEditor(game::TileMap&, View&, IRect rect);

		void setTileGroup(const TileGroup *tile_group) { m_tile_group = tile_group; }
		void setNewTile(const game::Tile *tile) { m_new_tile = tile; }
			
		void drawContents(Renderer2D&) const override;
		void onInput(const InputState&) override;
		bool onMouseDrag(const InputState&, int2 start, int2 current, int key, int is_final) override;

		enum Mode {
			mode_selecting_normal,
			mode_selecting_union,
			mode_selecting_intersection,
			mode_selecting_difference,

			mode_placing,
			mode_replacing,

			mode_placing_random,
			mode_replacing_random,

			mode_filling,

			mode_occluders,

			mode_count,
		};

		bool isSelecting() const
			{ return m_mode >= mode_selecting_normal && m_mode <= mode_selecting_difference; }
		bool isReplacing() const
			{ return m_mode == mode_replacing || m_mode == mode_replacing_random; }
		bool isPlacing() const
			{ return m_mode >= mode_placing && m_mode <= mode_replacing; }
		bool isPlacingRandom() const
			{ return m_mode >= mode_placing_random && m_mode <= mode_replacing_random; }
		bool isFilling() const
			{ return m_mode == mode_filling; }
		bool isChangingOccluders() const
			{ return m_mode == mode_occluders; }

		void setMode(Mode);
		Mode mode() const { return m_mode; }

		static const char **modeStrings();
		
		float m_dirty_percent;

	private:
		Mode m_mode;

		View &m_view;
		game::TileMap &m_tile_map;
		const TileGroup *m_tile_group;

		const game::Tile *m_new_tile;
		vector<int> m_selected_ids;
		
		void fill(const IBox &fill_box, bool is_randomized = false, int group_id = -1);
		void fillHoles(int main_group_id, const IBox &fill_box);
		void removeAll(const IBox &box);

		int findAt(const int3 &pos) const;

		void drawBoxHelpers(Renderer2D&, const IBox &box) const;
		const IBox computeCursor(const int2 &start, const int2 &end) const;
		
		IBox m_selection;
		int3 m_move_offset;
		int m_cursor_offset;
		int m_current_occluder;
		int m_mouseover_tile_id;
		bool m_is_selecting, m_is_moving;
		bool m_is_moving_vertically;
	};

	using PTilesEditor = shared_ptr<TilesEditor>;

}


#endif

