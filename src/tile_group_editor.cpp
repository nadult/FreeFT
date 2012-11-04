#include "tile_group_editor.h"
#include "tile_selector.h"
#include "gfx/device.h"
#include "tile_group.h"
#include <algorithm>
#include <cstring>
#include <tuple>

using namespace gfx;

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
	:ui::Window(rect, Color(0, 0, 0)), m_tile_list(rect.Width(), 2) {
	m_view = clippedRect();

	m_tile_group = nullptr;

	m_font = Font::mgr["font1"];
	m_font_texture = Font::tex_mgr["font1"];
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
}

void TileGroupEditor::onInput(int2 mouse_pos) {
	Assert(m_tile_group);

	if(IsKeyDown(Key_space)) {
		m_mode = (m_mode == mAddRemove ? mModify : mAddRemove);
		updateSelector();
		return;
	}
	
	int wheel = GetMouseWheelMove();
	if(wheel) {
		m_offset[m_mode].y -= wheel * rect().Height() / 16;
		m_offset[m_mode].y = Clamp(m_offset[m_mode].y, 0, m_tile_list.m_height);
	}

	if(m_mode == mModify) {
		const ui::TileList::Entry *entry = m_tile_list.find(mouse_pos + m_offset[m_mode]);

		if(IsKeyDown('G') && entry && m_selected_group_id != -1) {
			m_tile_group->setEntryGroup(m_tile_group->findEntry(entry->m_tile),
				entry->m_group_id == m_selected_group_id? m_tile_group->groupCount() : m_selected_group_id);
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
				if(IsKeyDown(actions[a].key) || IsKeyDown(Key_kp_5))
					m_tile_group->setGroupSurface(m_selected_group_id, actions[a].side, m_selected_surface_id);
		}

		for(int n = 0; n <= 9; n++)
			if(IsKeyDown('0' + n))
				m_selected_surface_id = n;
		if(IsKeyDown('-'))
			m_selected_surface_id = -1;
	}
}
	
bool TileGroupEditor::onMouseDrag(int2 start, int2 current, int key, bool is_final) {
	if(key == 2 || (IsKeyPressed(Key_lctrl) && key == 0)) {
		m_offset[m_mode].y -= GetMouseMove().y;
		m_offset[m_mode].y = Clamp(m_offset[m_mode].y, 0, m_tile_list.m_height);
		return true;
	}
	else if(key == 0) {
		const ui::TileList::Entry *entry = m_tile_list.find(current + m_offset[m_mode]);

		if(m_mode == mAddRemove && entry) {
			int selection_mode = m_tile_group->findEntry(entry->m_tile) == -1? 1 : -1;
			if(!m_selection_mode)
				m_selection_mode = selection_mode;
			if(m_selection_mode == selection_mode) {
				if(selection_mode == -1)
					m_tile_group->removeEntry(m_tile_group->findEntry(entry->m_tile));
				else
					m_tile_group->addEntry(entry->m_tile);
				entry = nullptr;
				m_tile_list.update();
			}
		
		}
		if(m_mode == mModify) {
			m_selected_group_id = entry? entry->m_group_id : -1;
		}

		if(is_final)
			m_selection_mode = 0;
		return true;
	}

	return false;
}

void TileGroupEditor::drawContents() const {
	for(int n = 0; n < m_tile_group->entryCount(); n++)
	   m_tile_group->entryTile(n)->m_temp = n;

	for(int n = 0; n < (int)m_tile_list.size(); n++) {
		const ui::TileList::Entry &entry = m_tile_list[n];
		entry.m_is_selected = m_mode == mAddRemove?
			m_tile_group->isValidEntryId(entry.m_tile->m_temp, entry.m_tile) :
			entry.m_group_id == m_selected_group_id;
		entry.m_tile->Draw(entry.m_pos - entry.m_tile->GetBounds().min - m_offset[m_mode]);
	}
	
	DTexture::Bind0();
	for(int n = 0; n < (int)m_tile_list.size(); n++) {
		const ui::TileList::Entry &entry = m_tile_list[n];
		if(!entry.m_is_selected)
			continue;
		
		int2 pos = entry.m_pos - m_offset[m_mode];
		//LookAt(-clippedRect().min - pos - entry.m_tile->offset);
		//IBox box(int3(0, 0, 0), entry.m_tile->bbox);
		//DrawBBox(box, Color(255, 255, 255));
		DrawRect(IRect(pos, pos + entry.m_size));
	}

	if(m_mode == mModify && m_selected_group_id != -1) {	
		IRect edit_rect(clippedRect().max - int2(280, 250), clippedRect().max);
		int2 center = edit_rect.Center();

		LookAt(-center);
		DrawQuad(-edit_rect.Size() / 2, edit_rect.Size(), Color(80, 80, 80));
		DrawBBox(IBox({-9, 0, -9}, {9, 1, 9}), Color(255, 255, 255));

		m_font_texture->Bind();
		m_font->SetSize(int2(35, 25));
		m_font->SetPos(int2(0, 0));

		char text[32];

		for(int n = 0; n < TileGroup::Group::sideCount; n++) {
			LookAt(-center - WorldToScreen(TileGroup::Group::s_side_offsets[n] * 9).xy());
			snprintf(text, sizeof(text), "%d", m_tile_group->groupSurface(m_selected_group_id, n));
			m_font->SetPos(int2(0, 0));
			m_font->Draw(text);
		}
			
		LookAt(-center +edit_rect.Size() / 2);
		m_font->SetPos(int2(0, 0));
		snprintf(text, sizeof(text), "next_surf:%d", m_selected_surface_id);
		m_font->Draw(text);

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

		m_font->SetPos(int2(0, 10));
		m_font->SetSize(int2(28, 18));
		LookAt(-int2(bottom_rect.max.x - 200, bottom_rect.min.y));

		for(int n = 0; n < COUNTOF(names); n++) {
			snprintf(text, sizeof(text), m_selected_surface_id == n? "%d: [%s]\n" : "%d: %s\n", n, names[n]);
			m_font->Draw(text);
		}*/
	}

}

void TileGroupEditor::setTarget(TileGroup* tile_group) {
	m_tile_group = tile_group;
	updateSelector();
}
