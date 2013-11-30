/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "editor/group_editor.h"
#include "editor/tile_group.h"
#include "gfx/font.h"
#include "gfx/device.h"
#include <algorithm>
#include <cstring>
#include <tuple>

//TODO: fixme
#ifdef _WIN32
const char* strcasestr(const char *a, const char *b) { return strstr(a, b); }
#endif

using namespace gfx;
using namespace game;

namespace ui {

namespace {

	struct TileGroupModel: public ui::TileListModel {
		TileGroupModel(TileGroup &tile_group) :m_tile_group(tile_group) { }
		int size() const { return m_tile_group.entryCount(); }
		const Tile *get(int idx, int &group_id) const {
			group_id = m_tile_group.entryGroup(idx);
			return m_tile_group.entryTile(idx);
		}

		TileGroup &m_tile_group;
	};

}

	GroupEditor::GroupEditor(IRect rect)
		:ui::Window(rect, Color(0, 0, 0)), m_tile_list(rect.width(), 2) {
		m_view = clippedRect();

		m_tile_group = nullptr;
		m_current_entry = nullptr;

		m_font = Font::mgr[s_font_names[1]];
		m_mode = mAddRemove;
		memset(m_offset, 0, sizeof(m_offset));
		m_selected_group_id = 0;
		m_selected_surface_id = -1;
		m_select_mode = 0;
		m_selection_mode = 0;
		m_tile_filter = TileFilter::floors;

		updateSelector();
	}

	void GroupEditor::setTileFilter(TileFilter::Type filter) {
		m_tile_filter = filter;
		updateSelector();
	}

	void GroupEditor::updateSelector() {
		PTileListModel model =
			m_mode == mAddRemove? allTilesModel() :
				m_tile_group? new TileGroupModel(*m_tile_group) : nullptr;

		m_current_entry = nullptr;
		m_tile_list.setModel(filteredTilesModel(model, m_tile_filter));

		int2 pos(0, -m_offset[m_mode].y);
		int2 size(rect().width(), m_tile_list.m_height + (m_mode == mAddRemove? 0 : rect().height() / 2));

		setInnerRect(IRect(pos, pos + size));
	}

	void GroupEditor::onInput(int2 mouse_pos) {
		ASSERT(m_tile_group);

		if(isKeyDown(Key_space)) {
			m_offset[m_mode] = innerOffset();
			m_mode = (m_mode == mAddRemove ? mModify : mAddRemove);
			updateSelector();
			return;
		}
			
		const ui::TileList::Entry *entry = m_tile_list.find(mouse_pos + innerOffset());
		m_current_entry = entry;
		
		if(m_mode == mModify) {
			if(isKeyDown('D') && entry) {
				int entry_id = m_tile_group->findEntry(entry->tile);
				m_tile_group->setEntryDirty(entry_id, !m_tile_group->isEntryDirty(entry_id));
			}
			if(isKeyDown('G') && entry && m_selected_group_id != -1) {
				m_tile_group->setEntryGroup(m_tile_group->findEntry(entry->tile),
					entry->group_id == m_selected_group_id? m_tile_group->groupCount() : m_selected_group_id);
				m_tile_list.update();
			}
			if(isKeyDown('A') && entry && m_selected_group_id == entry->group_id) {
				enum { subgroup_count = 3 };
				const char *infixes[subgroup_count] = {
					"CONCAVE_",
					"CONVEX_",
					"SIDE_",
				};

				int subgroup_id[subgroup_count] = {-1, -1, -1};

				for(int n = 0; n < m_tile_group->entryCount(); n++) {
					if(m_tile_group->entryGroup(n) == m_selected_group_id) {
						const char *name = m_tile_group->entryTile(n)->resourceName();

						for(int s = 0; s < subgroup_count; s++)
							if(strcasestr(name, infixes[s])) {
								if(subgroup_id[s] == -1)
									subgroup_id[s] = m_tile_group->groupCount();
								m_tile_group->setEntryGroup(n, subgroup_id[s]);
								break;
							}
					}
				}
				m_tile_list.update();
			}

			struct { KeyId key; int side; } actions[] = {
				{ Key_kp_1, 0 },
				{ Key_kp_2, 1 },
				{ Key_kp_3, 2 },
				{ Key_kp_6, 3 },
				{ Key_kp_9, 4 },
				{ Key_kp_8, 5 },
				{ Key_kp_7, 6 },
				{ Key_kp_4, 7 } };

			if(m_selected_group_id != -1) {
				for(int a = 0; a < COUNTOF(actions); a++)
					if(isKeyDown(actions[a].key) || isKeyDown(Key_kp_5))
						m_tile_group->setGroupSurface(m_selected_group_id, actions[a].side, m_selected_surface_id);
			}

			for(int n = 0; n <= 9; n++)
				if(isKeyDown('0' + n))
					m_selected_surface_id = n;
			if(isKeyDown('-'))
				m_selected_surface_id = -1;
		}
	}
		
	bool GroupEditor::onMouseDrag(int2 start, int2 current, int key, int is_final) {
		if(key == 0) {
			const ui::TileList::Entry *entry = m_tile_list.find(current + innerOffset());
			m_current_entry = entry;

			if(m_mode == mAddRemove && entry) {
				int selection_mode = m_tile_group->findEntry(entry->tile) == -1? 1 : -1;
				if(!m_selection_mode)
					m_selection_mode = selection_mode;
				if(m_selection_mode == selection_mode) {
					if(selection_mode == -1)
						m_tile_group->removeEntry(m_tile_group->findEntry(entry->tile));
					else
						m_tile_group->addEntry(entry->tile);
					entry = nullptr;
					m_tile_list.update();
				}
			
			}
			if(m_mode == mModify) {
				m_selected_group_id = entry? entry->group_id : -1;
			}

			if(is_final)
				m_selection_mode = 0;
			return true;
		}

		return false;
	}

	void GroupEditor::drawContents() const {
		int2 offset = innerOffset();

		for(int n = 0; n < m_tile_group->entryCount(); n++)
		   m_tile_group->entryTile(n)->m_temp = n;

		IRect clip_rect(int2(0, 0), clippedRect().size());

		for(int n = 0; n < (int)m_tile_list.size(); n++) {
			const ui::TileList::Entry &entry = m_tile_list[n];
			entry.is_selected = m_mode == mAddRemove?
				m_tile_group->isValidEntryId(entry.tile->m_temp, entry.tile) :
				entry.group_id == m_selected_group_id;

			IRect tile_rect = entry.tile->rect();
			int2 pos = entry.pos - tile_rect.min - offset;

			if(areOverlapping(clip_rect, tile_rect + pos))
				entry.tile->draw(pos);
		}
		
		DTexture::bind0();
		for(int n = 0; n < (int)m_tile_list.size(); n++) {
			const ui::TileList::Entry &entry = m_tile_list[n];
			if(!entry.is_selected)
				continue;
		
			Color col = m_tile_group->isEntryDirty(entry.tile->m_temp)? Color::red : Color::white;	
			int2 pos = entry.pos - offset;
			drawRect(IRect(pos, pos + entry.size), col);
		}

		if(m_mode == mModify && m_selected_group_id != -1) {	
			IRect edit_rect(clippedRect().max - int2(280, 250), clippedRect().max - int2(5, 0));
			int2 center = edit_rect.center();

			lookAt(-center);
			drawQuad(-edit_rect.size() / 2, edit_rect.size(), Color(80, 80, 80));
			drawBBox(IBox({-9, 0, -9}, {9, 1, 9}), Color(255, 255, 255));

			PFont font = gfx::Font::mgr[s_font_names[0]];

			for(int n = 0; n < TileGroup::Group::side_count; n++) {
				lookAt(-center - worldToScreen(TileGroup::Group::s_side_offsets[n] * 9));
				font->draw(int2(0, 0), Color::white, "%d", m_tile_group->groupSurface(m_selected_group_id, n));
			}
				
			lookAt(-center +edit_rect.size() / 2);
			font->draw(int2(0, 0), Color::white, "setting surface: %d", m_selected_surface_id);

			/*
			const char *names[] = {
				"",
				"snow",
				"gravel",
				"dirt",
				"grass",
				"mud",
				"mud_cracked",
				"sand",
				"green goo",
			};

			lookAt(-int2(bottom_rect.max.x - 200, bottom_rect.min.y));
			for(int n = 0; n < COUNTOF(names); n++)
				font->draw(int2(0, 10), Color::white,
						m_selected_surface_id == n? "%d: [%s]\n" : "%d: %s\n", n, names[n]); */
			lookAt(-clippedRect().min);
		}

		if(m_current_entry)
			m_font->drawShadowed(int2(5, height() - 20), Color::white, Color::black, "%s",
					m_current_entry->tile->resourceName());
	}

	void GroupEditor::setTarget(TileGroup* tile_group) {
		m_tile_group = tile_group;
		updateSelector();
	}

}
