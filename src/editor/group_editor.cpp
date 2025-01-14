// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "editor/group_editor.h"
#include "editor/tile_group.h"
#include "gfx/drawing.h"
#include <algorithm>
#include <cstring>
#include <tuple>

using namespace game;

namespace ui {

namespace {

	struct TileGroupModel : public ui::TileListModel {
		TileGroupModel(TileGroup &tile_group) : m_tile_group(tile_group) {}
		int size() const { return m_tile_group.entryCount(); }
		const Tile *get(int idx, int &group_id) const {
			group_id = m_tile_group.entryGroup(idx);
			return m_tile_group.entryTile(idx);
		}

		TileGroup &m_tile_group;
	};

}

GroupEditor::GroupEditor(IRect rect)
	: ui::Window(rect, Color(0, 0, 0)), m_tile_list(rect.width(), 2),
	  m_font(res::getFont(WindowStyle::fonts[1])) {
	m_view = clippedRect();

	m_tile_group = nullptr;
	m_current_entry = nullptr;

	m_mode = mAddRemove;
	memset(m_offset, 0, sizeof(m_offset));
	m_selected_group_id = 0;
	m_selected_surface_id = -1;
	m_select_mode = 0;
	m_selection_mode = 0;
	m_tile_filter = TileFilter::floors;

	updateSelector();
}

void GroupEditor::setTileFilter(TileFilter filter) {
	m_tile_filter = filter;
	updateSelector();
}

void GroupEditor::updateSelector() {
	PTileListModel model = m_mode == mAddRemove ? allTilesModel() :
						   m_tile_group			? make_shared<TileGroupModel>(*m_tile_group) :
												  nullptr;

	m_current_entry = nullptr;
	m_tile_list.setModel(filteredTilesModel(model, m_tile_filter));

	int2 pos(0, -m_offset[m_mode].y);
	int2 size(rect().width(),
			  m_tile_list.m_height + (m_mode == mAddRemove ? 0 : rect().height() / 2));

	setInnerRect(IRect(pos, pos + size));
}

void GroupEditor::onInput(const InputState &state) {
	ASSERT(m_tile_group);

	if(state.isKeyDown(InputKey::space)) {
		m_offset[m_mode] = innerOffset();
		m_mode = (m_mode == mAddRemove ? mModify : mAddRemove);
		updateSelector();
		return;
	}

	const ui::TileList::Entry *entry = m_tile_list.find(state.mousePos() + innerOffset());
	m_current_entry = entry;

	if(m_mode == mModify) {
		if(state.isKeyDown('D') && entry) {
			int entry_id = m_tile_group->findEntry(entry->tile);
			m_tile_group->setEntryDirty(entry_id, !m_tile_group->isEntryDirty(entry_id));
		}
		if(state.isKeyDown('G') && entry && m_selected_group_id != -1) {
			m_tile_group->setEntryGroup(m_tile_group->findEntry(entry->tile),
										entry->group_id == m_selected_group_id ?
											m_tile_group->groupCount() :
											m_selected_group_id);
			m_tile_list.update();
		}
		if(state.isKeyDown('A') && entry && m_selected_group_id == entry->group_id) {
			static constexpr int subgroup_count = 3;
			const char *infixes[subgroup_count] = {
				"CONCAVE_",
				"CONVEX_",
				"SIDE_",
			};

			int subgroup_id[subgroup_count] = {-1, -1, -1};

			for(int n = 0; n < m_tile_group->entryCount(); n++) {
				if(m_tile_group->entryGroup(n) == m_selected_group_id) {
					const char *name = m_tile_group->entryTile(n)->resourceName().c_str();

					for(int s = 0; s < subgroup_count; s++)
						if(!equalIgnoreCase(name, infixes[s])) {
							if(subgroup_id[s] == -1)
								subgroup_id[s] = m_tile_group->groupCount();
							m_tile_group->setEntryGroup(n, subgroup_id[s]);
							break;
						}
				}
			}
			m_tile_list.update();
		}

		struct {
			int key;
			int side;
		} actions[] = {{InputKey::kp_1, 0}, {InputKey::kp_2, 1}, {InputKey::kp_3, 2},
					   {InputKey::kp_6, 3}, {InputKey::kp_9, 4}, {InputKey::kp_8, 5},
					   {InputKey::kp_7, 6}, {InputKey::kp_4, 7}};

		if(m_selected_group_id != -1) {
			for(int a = 0; a < arraySize(actions); a++)
				if(state.isKeyDown(actions[a].key) || state.isKeyDown(InputKey::kp_5))
					m_tile_group->setGroupSurface(m_selected_group_id, actions[a].side,
												  m_selected_surface_id);
		}

		for(int n = 0; n <= 9; n++)
			if(state.isKeyDown('0' + n))
				m_selected_surface_id = n;
		if(state.isKeyDown('-'))
			m_selected_surface_id = -1;
	}
}

bool GroupEditor::onMouseDrag(const InputState &, int2 start, int2 current, int key, int is_final) {
	if(key == 0) {
		const ui::TileList::Entry *entry = m_tile_list.find(current + innerOffset());
		m_current_entry = entry;

		if(m_mode == mAddRemove && entry) {
			int selection_mode = m_tile_group->findEntry(entry->tile) == -1 ? 1 : -1;
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
			m_selected_group_id = entry ? entry->group_id : -1;
		}

		if(is_final)
			m_selection_mode = 0;
		return true;
	}

	return false;
}

void GroupEditor::drawContents(Canvas2D &out) const {
	int2 offset = innerOffset();

	for(int n = 0; n < m_tile_group->entryCount(); n++)
		m_tile_group->entryTile(n)->m_temp = n;

	IRect clip_rect(int2(0, 0), clippedRect().size());

	for(int n = 0; n < (int)m_tile_list.size(); n++) {
		const ui::TileList::Entry &entry = m_tile_list[n];
		entry.is_selected = m_mode == mAddRemove ?
								m_tile_group->isValidEntryId(entry.tile->m_temp, entry.tile) :
								entry.group_id == m_selected_group_id;

		IRect tile_rect = entry.tile->rect();
		int2 pos = entry.pos - tile_rect.min() - offset;

		if(areOverlapping(clip_rect, tile_rect + pos))
			entry.tile->draw(out, pos);
	}

	for(int n = 0; n < (int)m_tile_list.size(); n++) {
		const ui::TileList::Entry &entry = m_tile_list[n];
		if(!entry.is_selected)
			continue;

		Color col = m_tile_group->isEntryDirty(entry.tile->m_temp) ? ColorId::red : ColorId::white;
		int2 pos = entry.pos - offset;
		out.addRect(IRect(pos, pos + entry.size), col);
	}

	if(m_mode == mModify && m_selected_group_id != -1) {
		IRect edit_rect(clippedRect().max() - int2(280, 250), clippedRect().max() - int2(5, 0));
		int2 center = edit_rect.center();

		out.setViewPos(-center);
		int2 half_size = edit_rect.size() / 2;
		out.addFilledRect(IRect(-half_size, half_size), FColor(0.3f, 0.3f, 0.3f));
		drawBBox(out, IBox({-9, 0, -9}, {9, 1, 9}), ColorId::white);

		auto &font = res::getFont(WindowStyle::fonts[0]);

		for(int n = 0; n < TileGroup::Group::side_count; n++) {
			out.setViewPos(-center - worldToScreen(TileGroup::Group::s_side_offsets[n] * 9));
			font.draw(out, float2(0, 0), {ColorId::white},
					  format("%", m_tile_group->groupSurface(m_selected_group_id, n)));
		}

		out.setViewPos(-center + edit_rect.size() / 2);
		font.draw(out, float2(0, 0), {ColorId::white},
				  format("setting surface: %", m_selected_surface_id));

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

			out.setViewPos(-int2(bottom_rect.ex() - 200, bottom_rect.y()));
			for(int n = 0; n < arraySize(names); n++)
				font.draw(int2(0, 10), ColorId::white,
						m_selected_surface_id == n? "%d: [%s]\n" : "%d: %s\n", n, names[n]); */
		out.setViewPos(-clippedRect().min());
	}

	if(m_current_entry)
		m_font.draw(out, float2(5, height() - 20), {ColorId::white, ColorId::black},
					format("%", m_current_entry->tile->resourceName()));
}

void GroupEditor::setTarget(TileGroup *tile_group) {
	m_tile_group = tile_group;
	updateSelector();
}

}
