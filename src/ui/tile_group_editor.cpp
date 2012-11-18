#include "ui/tile_group_editor.h"
#include "gfx/device.h"
#include "tile_group.h"
#include <algorithm>
#include <cstring>
#include <tuple>

using namespace gfx;

namespace ui {

namespace {

	struct TileGroupModel: public ui::TileListModel {
		TileGroupModel(TileGroup &tile_group) :m_tile_group(tile_group) { }
		int size() const { return m_tile_group.entryCount(); }
		const gfx::Tile *get(int idx, int &group_id) const {
			group_id = m_tile_group.entryGroup(idx);
			return m_tile_group.entryTile(idx);
		}

		TileGroup &m_tile_group;
	};

}

	TileGroupEditor::TileGroupEditor(IRect rect)
		:ui::Window(rect, Color(0, 0, 0)), m_tile_list(rect.width(), 2) {
		m_view = clippedRect();

		m_tile_group = nullptr;

		m_font = Font::mgr["times_16"];
		m_mode = mAddRemove;
		memset(m_offset, 0, sizeof(m_offset));
		m_selected_group_id = 0;
		m_selected_surface_id = -1;
		m_select_mode = 0;
		m_selection_mode = 0;

		updateSelector();
	}

	void TileGroupEditor::updateSelector() {
		if(m_mode == mAddRemove)
			m_tile_list.setModel(new ui::AllTilesModel);
		else
			m_tile_list.setModel(m_tile_group? new TileGroupModel(*m_tile_group) : nullptr);

		int2 pos(0, -m_offset[m_mode].y);
		int2 size(rect().width(), m_tile_list.m_height + (m_mode == mAddRemove? 0 : rect().height() / 2));

		setInnerRect(IRect(pos, pos + size));
	}

	void TileGroupEditor::onInput(int2 mouse_pos) {
		ASSERT(m_tile_group);

		if(isKeyDown(Key_space)) {
			m_offset[m_mode] = innerOffset();
			m_mode = (m_mode == mAddRemove ? mModify : mAddRemove);
			updateSelector();
			return;
		}
		
		if(m_mode == mModify) {
			const ui::TileList::Entry *entry = m_tile_list.find(mouse_pos + innerOffset());

			if(isKeyDown('G') && entry && m_selected_group_id != -1) {
				m_tile_group->setEntryGroup(m_tile_group->findEntry(entry->tile),
					entry->group_id == m_selected_group_id? m_tile_group->groupCount() : m_selected_group_id);
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
		
	bool TileGroupEditor::onMouseDrag(int2 start, int2 current, int key, int is_final) {
		if(key == 0) {
			const ui::TileList::Entry *entry = m_tile_list.find(current + innerOffset());

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

	void TileGroupEditor::drawContents() const {
		int2 offset = innerOffset();

		for(int n = 0; n < m_tile_group->entryCount(); n++)
		   m_tile_group->entryTile(n)->m_temp = n;

		for(int n = 0; n < (int)m_tile_list.size(); n++) {
			const ui::TileList::Entry &entry = m_tile_list[n];
			entry.is_selected = m_mode == mAddRemove?
				m_tile_group->isValidEntryId(entry.tile->m_temp, entry.tile) :
				entry.group_id == m_selected_group_id;
			entry.tile->draw(entry.pos - entry.tile->GetBounds().min - offset);
		}
		
		DTexture::bind0();
		for(int n = 0; n < (int)m_tile_list.size(); n++) {
			const ui::TileList::Entry &entry = m_tile_list[n];
			if(!entry.is_selected)
				continue;
			
			int2 pos = entry.pos - offset;
			//lookAt(-clippedRect().min - pos - entry.tile->offset);
			//IBox box(int3(0, 0, 0), entry.tile->bbox);
			//drawBBox(box, Color(255, 255, 255));
			drawRect(IRect(pos, pos + entry.size));
		}

		if(m_mode == mModify && m_selected_group_id != -1) {	
			IRect edit_rect(clippedRect().max - int2(280, 250), clippedRect().max - int2(5, 0));
			int2 center = edit_rect.center();

			lookAt(-center);
			drawQuad(-edit_rect.size() / 2, edit_rect.size(), Color(80, 80, 80));
			drawBBox(IBox({-9, 0, -9}, {9, 1, 9}), Color(255, 255, 255));

			for(int n = 0; n < TileGroup::Group::sideCount; n++) {
				lookAt(-center - worldToScreen(TileGroup::Group::s_side_offsets[n] * 9));
				m_font->draw(int2(0, 0), Color::white, "%d", m_tile_group->groupSurface(m_selected_group_id, n));
			}
				
			lookAt(-center +edit_rect.size() / 2);
			m_font->draw(int2(0, 0), Color::white, "setting surface: %d", m_selected_surface_id);

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
				m_font->draw(int2(0, 10), Color::white,
						m_selected_surface_id == n? "%d: [%s]\n" : "%d: %s\n", n, names[n]); */
		}

	}

	void TileGroupEditor::setTarget(TileGroup* tile_group) {
		m_tile_group = tile_group;
		updateSelector();
	}

}
