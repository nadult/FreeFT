#include "tile_group_editor.h"
#include "gfx/device.h"
#include "tile_group.h"
#include <algorithm>
#include <cstring>
#include <tuple>

using namespace gfx;



FloorTileGroupEditor::FloorTileGroupEditor(int2 res) :m_view(0, 0, res.x, res.y) {
	m_tiles = nullptr;
	m_tile_group = nullptr;
	m_selected_tile = nullptr;

	m_font = Font::mgr["font1"];
	m_font_texture = Font::tex_mgr["font1"];
	m_mode = mAddRemove;
	memset(m_offset, 0, sizeof(m_offset));
	m_selected_match_id = 0;
	m_selected_surface_id = -1;
	m_select_mode = 0;
}

void FloorTileGroupEditor::loop() {
	Assert(m_tiles && m_tile_group);

	if(IsKeyDown(Key_space)) {
		m_mode = (m_mode == mAddRemove ? mModify : mAddRemove);
		m_selected_tile = nullptr;
	}

	IRect top_rect = m_view, bottom_rect = m_view;
	if(m_mode == mModify)
		bottom_rect.min.y = top_rect.max.y = top_rect.min.y + (top_rect.Height()) * 3 / 5;
	else
		bottom_rect.min.y = top_rect.max.y;

	int2 mouse_pos = GetMousePos();
	bool mouse_is_up = top_rect.IsInside(mouse_pos);

	if(mouse_is_up) {
		if(IsKeyPressed(Key_lctrl) || IsMouseKeyPressed(2))
			m_offset[m_mode] += GetMouseMove().y;
		m_offset[m_mode] += GetMouseWheelMove() * m_view.Height() / 8;
		if(m_offset[m_mode] > 100)
			m_offset[m_mode] = 100;
	}
	else {
		if(IsMouseKeyPressed(2))
			m_offset[2] += GetMouseMove().x;
		if(m_offset[2] > 100)
			m_offset[2] = 100;
	}


	struct TileInstance {
		IRect rect;
		const gfx::Tile *tile;
		int index;
	};

	// Only visible tiles
	vector<TileInstance> instances; {
		instances.reserve(m_tiles->size());
		int2 pos(0, m_offset[m_mode]);
		int maxy = 0;

		if(m_mode == mAddRemove) {
			for(int n = 0; n < (int)m_tiles->size(); n++) {
				const gfx::Tile *tile = &(*m_tiles)[n];
				IRect bounds = tile->GetBounds();

				if(bounds.Width() + pos.x > m_view.Width() && pos.x != 0) {
					pos = int2(0, pos.y + maxy);
					if(pos.y >= top_rect.Height())
						break;
					maxy = 0;
				}
			
				if(pos.y + bounds.max.y >= top_rect.min.y)
					instances.push_back({IRect(pos, pos + bounds.Size()), tile, n});
	
				pos.x += bounds.Width() + 2;
				maxy = Max(maxy, bounds.Height() + 2);
			}
		}
		else if(m_mode == mModify) {
			struct TEntry {
				int id, gid, gsize;
				bool operator<(const TEntry &rhs) const {
					return gsize == rhs.gsize? gid == rhs.gid? id < rhs.id : gid < rhs.gid : gsize > rhs.gsize;
				}
			};

			vector<TEntry> entries;
			entries.resize(m_tile_group->entryCount());
			for(int e = 0; e < m_tile_group->entryCount(); e++) {
				int group_id = m_tile_group->entryGroup(e);
				entries[e] = TEntry{e, group_id, m_tile_group->groupEntryCount(group_id)};
			}

			stable_sort(entries.begin(), entries.end());
			int last_group = -1, last_group_size = 0;

			for(int e = 0; e < (int)entries.size(); e++) {
				const gfx::Tile *tile = m_tile_group->entryTile(entries[e].id);
				int group_id = entries[e].gid;
				int group_size = entries[e].gsize;
				IRect bounds = tile->GetBounds();

				if( (bounds.Width() + pos.x > m_view.Width() && pos.x != 0) || (e && group_id != last_group &&
							(last_group_size != 1 || group_size != 1)) ) {
					pos = int2(0, pos.y + maxy);
					if(pos.y >= top_rect.Height())
						break;
					maxy = 0;
				}
				
				if(pos.y + bounds.max.y >= top_rect.min.y)
					instances.push_back({IRect(pos, pos + bounds.Size()), tile, e});

				pos.x += bounds.Width() + 2;
				maxy = Max(maxy, bounds.Height() + 2);
				last_group = group_id;
				last_group_size = group_size;
			}
		}
	}
		
	for(int n = 0; n < (int)m_tiles->size(); n++)
		(*m_tiles)[n].m_temp = 0;

	int mouse_over_id = -1;
	for(int n = 0; n < (int)instances.size(); n++)
		if(instances[n].rect.IsInside(mouse_pos)) {
			mouse_over_id = n;
			break;
		}

	if(m_mode == mAddRemove) {
		if(mouse_is_up && mouse_over_id != -1) {
			int entry_id = m_tile_group->findEntry(instances[mouse_over_id].tile);

			if(IsMouseKeyDown(0)) {
				m_select_mode = entry_id == -1? -1 : 1;

				if(entry_id == -1)
					m_tile_group->addEntry(instances[mouse_over_id].tile);
				else
					m_tile_group->removeEntry(m_tile_group->findEntry(instances[mouse_over_id].tile));
			}
			else if(IsMouseKeyPressed(0)) {
				if(entry_id == -1 && m_select_mode == -1)
					m_tile_group->addEntry(instances[mouse_over_id].tile);
				else if(m_select_mode == 1)
					m_tile_group->removeEntry(m_tile_group->findEntry(instances[mouse_over_id].tile));
			}
		}
	
		for(int e = 0; e < m_tile_group->entryCount(); e++)
			m_tile_group->entryTile(e)->m_temp = 1;
	}
	else {
		if(IsMouseKeyDown(0) && mouse_is_up) {
			m_selected_tile = mouse_over_id == -1? nullptr : instances[mouse_over_id].tile;
			m_selected_match_id = 0;
			m_offset[2] = 0;
		}
		int selected_entry_id = m_selected_tile? m_tile_group->findEntry(m_selected_tile) : -1;
		int selected_group_id = selected_entry_id == -1? -1 : m_tile_group->entryGroup(selected_entry_id);

		struct { KeyId key; int side; } actions[] = {
			{ Key_kp_1, 0 },
			{ Key_kp_2, 1 },
			{ Key_kp_3, 2 },
			{ Key_kp_6, 3 },
			{ Key_kp_9, 4 },
			{ Key_kp_8, 5 },
			{ Key_kp_7, 6 },
			{ Key_kp_4, 7 } };

		if(selected_entry_id != -1) {
			for(int a = 0; a < COUNTOF(actions); a++)
				if(IsKeyDown(actions[a].key) || IsKeyDown(Key_kp_5))
					m_tile_group->setGroupSurface(selected_group_id, actions[a].side, m_selected_surface_id);
		}

		for(int n = 0; n <= 9; n++)
			if(IsKeyDown('0' + n))
				m_selected_surface_id = n;
		if(IsKeyDown('-'))
			m_selected_surface_id = -1;

		if(IsKeyDown('G') && mouse_is_up && mouse_over_id != -1 && selected_entry_id != -1) {
			const gfx::Tile *mouse_over_tile = instances[mouse_over_id].tile;
			int entry_id = m_tile_group->findEntry(mouse_over_tile);

			if(m_tile_group->entryGroup(entry_id) == selected_group_id)
				m_tile_group->setEntryGroup(entry_id, m_tile_group->groupCount());
			else
				m_tile_group->setEntryGroup(entry_id, m_tile_group->entryGroup(selected_entry_id));
		}
		if(selected_entry_id != -1)	{
			for(int e = 0; e < m_tile_group->entryCount(); e++)
				m_tile_group->entryTile(e)->m_temp = m_tile_group->entryGroup(e) == selected_group_id;
		}
		//TODO: brać pod uwagę że podczas normalnego rysowania tile będzie przesunięty o boundsy
	}

	LookAt(m_view.min);

	DTexture::Bind0();
	SetScissorTest(true);
	SetScissorRect(bottom_rect);
	Clear(Color(128, 48, 0));
	
	SetScissorRect(top_rect);

	for(int n = 0; n < (int)instances.size(); n++) {
		const TileInstance &inst = instances[n];
		inst.tile->Draw(inst.rect.min - inst.tile->GetBounds().min);
	}

	DTexture::Bind0();
	for(int n = 0; n < (int)instances.size(); n++) {
		const TileInstance &inst = instances[n];

		if(inst.tile->m_temp & 1)
			DrawRect(inst.rect, Color(255, 0, 0));
		if(inst.tile->m_temp & 2)
			DrawRect(inst.rect, Color(0, 0, 255));
		if(m_mode == mAddRemove? mouse_over_id == n : m_selected_tile == inst.tile)
			DrawRect(IRect(inst.rect.min - int2(1, 1), inst.rect.max + int2(1, 1)), Color(255, 255, 255));
	}
		
	m_font_texture->Bind();
	m_font->SetPos(int2(0, 25));
	m_font->SetSize(int2(25, 18));
	m_font->Draw(mouse_over_id != -1? instances[mouse_over_id].tile->name.c_str() : "");
	DTexture::Bind0();
	
	int selected_entry_id = m_selected_tile? m_tile_group->findEntry(m_selected_tile) : -1;
	int selected_group_id = selected_entry_id == -1? -1 : m_tile_group->entryGroup(selected_entry_id);

	if(m_mode == mModify && selected_group_id != -1) {	
		SetScissorRect(bottom_rect);
		LookAt(-bottom_rect.Center());
		DrawBBox(IBox({-9, 0, -9}, {9, 1, 9}), Color(255, 255, 255));

		int3 offsets[FloorTileGroup::Group::sideCount] = {
			{  0, 0,  1 },
			{  1, 0,  1 },
			{  1, 0,  0 },
			{  1, 0, -1 },
			{  0, 0, -1 },
			{ -1, 0, -1 },
			{ -1, 0,  0 },
	   		{ -1, 0,  1 } };
	
		m_font_texture->Bind();
		m_font->SetSize(int2(35, 25));
		m_font->SetPos(int2(0, 0));

		LookAt(-bottom_rect.min);
		char text[32];

		for(int n = 0; n < FloorTileGroup::Group::sideCount; n++) {
			LookAt(-bottom_rect.Center() - WorldToScreen(offsets[n] * 9));
			snprintf(text, sizeof(text), "%d", m_tile_group->groupSurface(selected_group_id, n));
			m_font->SetPos(int2(0, 0));
			m_font->Draw(text);
		}


		const char *names[] = {
			"",
			"snow",
			"gravel",
			"dirt",
			"grass",
			"mud",
			"mud_cracked",
			"sand",
		};

		m_font->SetPos(int2(0, 10));
		m_font->SetSize(int2(28, 18));
		LookAt(-int2(bottom_rect.max.x - 200, bottom_rect.min.y));

		for(int n = 0; n < COUNTOF(names); n++) {
			snprintf(text, sizeof(text), m_selected_surface_id == n? "%d: [%s]\n" : "%d: %s\n", n, names[n]);
			m_font->Draw(text);
		}
	}

	SetScissorTest(false);
}

void FloorTileGroupEditor::setSource(const vector<gfx::Tile> *tiles) {
	m_tiles = tiles;
}

void FloorTileGroupEditor::setTarget(FloorTileGroup* tile_group) {
	m_tile_group = tile_group;
}

int FloorTileGroupEditor::tileCount() const {
	return m_tiles? (int)m_tiles->size() : 0;
}

const gfx::Tile *FloorTileGroupEditor::getTile(int idx) const {
	return m_tiles? &(*m_tiles)[idx] : nullptr;
}

const char *FloorTileGroupEditor::title() const {
	return m_mode == mAddRemove? "FloorTileGroupEditor::adding/removing" : "TileGroupEditor::modifying";
}
