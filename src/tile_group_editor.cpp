#include "tile_group_editor.h"
#include "gfx/device.h"
#include "tile_group.h"
#include <algorithm>
#include <cstring>

using namespace gfx;

/*
float Compare(const Tile &a, const Tile &b) {
	int2 offset = b.offset - a.offset - int2(WorldToScreen(float3(a.bbox.x, 0.0f, 0.0f)));
	int2 aSize = a.texture.Size(), bSize = b.texture.Size();
	int2 size(Min(aSize.x, bSize.x), Min(aSize.y, bSize.y));

//	printf("Size: %dx%d\n", aSize.x, aSize.y);
//	printf("Offset: %dx%d\n", offset.x, offset.y);
	
	int2 min(offset.x < 0? -offset.x : 0, offset.y < 0? -offset.y : 0);
	int2 max(offset.x > 0? size.x - offset.x : size.x, offset.y > 0? size.y - offset.y : size.y);
	
	int missed = 0, matched = 0;
	for(int y = min.y; y < max.y; y++)
		for(int x = min.x; x < max.x; x++) {
			Color aPix = a.texture(x, y);
			Color bPix = b.texture(x + offset.x, y + offset.y);
				
			if(!aPix.a || !bPix.a)
				missed++;
			else {
				int rdist = abs((int)aPix.r - (int)bPix.r);
				int gdist = abs((int)aPix.g - (int)bPix.g);
				int bdist = abs((int)aPix.b - (int)bPix.b);

				int dist = Max(rdist, Max(gdist, bdist));
				if(dist < 20)
					matched++;	
			}
		}

	char text[256];
	snprintf(text, sizeof(text), "size: %dx%d  off: %dx%d  matches: %d/%d", size.x, size.y, offset.x, offset.y,
				matched, (max.x - min.x) * (max.y - min.y) - missed);
	font.Draw(text);

	return float(matched) / float((max.x - min.x) * (max.y - min.y));
}*/

TileGroupEditor::TileGroupEditor(int2 res) :m_view(0, 0, res.x, res.y) {
	m_tiles = nullptr;
	m_tile_group = nullptr;
	m_selected_tile = nullptr;

	m_font = Font::mgr["font1"];
	m_font_texture = Font::tex_mgr["font1"];
	m_mode = mAddRemove;
	memset(m_offset, 0, sizeof(m_offset));
	m_selected_match_id = 0;
}

void TileGroupEditor::loop() {
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
			vector<const TileGroup::Entry*> entries;
			entries.reserve(m_tile_group->size());

			for(int e = 0; e < m_tile_group->size(); e++)
				entries.push_back(&(*m_tile_group)[e]);

			stable_sort(entries.begin(), entries.end(), [](const TileGroup::Entry *a, const TileGroup::Entry *b)
					{ return a->m_tiles.size() > b->m_tiles.size(); } );

			for(int e = 0; e < (int)entries.size(); e++) {
				const TileGroup::Entry &entry = *entries[e];
				for(int t = 0; t < (int)entry.m_tiles.size(); t++) {
					const gfx::Tile *tile = entry.m_tiles[t];
					IRect bounds = tile->GetBounds();

					if( (bounds.Width() + pos.x > m_view.Width() && pos.x != 0) || (!instances.empty() &&
							e != instances.back().index && entries[e - 1]->m_tiles.size() > 1) ) {
						pos = int2(0, pos.y + maxy);
						if(pos.y >= top_rect.Height())
							break;
						maxy = 0;
					}
				
					if(pos.y + bounds.max.y >= top_rect.min.y)
						instances.push_back({IRect(pos, pos + bounds.Size()), tile, e});

					pos.x += bounds.Width() + 2;
					maxy = Max(maxy, bounds.Height() + 2);
				}
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
		if(IsMouseKeyDown(0) && mouse_is_up && mouse_over_id != -1) {
			int entry_id = m_tile_group->findEntry(instances[mouse_over_id].tile);
			if(entry_id == -1)
				m_tile_group->addTile(instances[mouse_over_id].tile);
			else
				m_tile_group->removeTile(instances[mouse_over_id].tile);
		}
	
		for(int e = 0; e < m_tile_group->size(); e++) {
			const TileGroup::Entry &entry = (*m_tile_group)[e];
			for(int t = 0; t < (int)entry.m_tiles.size(); t++)
				entry.m_tiles[t]->m_temp = 1;
		}
	}
	else {
		if(IsMouseKeyDown(0) && mouse_is_up) {
			m_selected_tile = mouse_over_id == -1? nullptr : instances[mouse_over_id].tile;
			m_selected_match_id = 0;
			m_offset[2] = 0;
		}
		int selected_entry_id = m_selected_tile? m_tile_group->findEntry(m_selected_tile) : -1;

		if(IsKeyDown('G') && mouse_is_up && mouse_over_id != -1 && selected_entry_id != -1) {
			const gfx::Tile *mouse_over_tile = instances[mouse_over_id].tile;
			int entry_id = m_tile_group->findEntry(mouse_over_tile);

			if(entry_id == selected_entry_id && mouse_over_tile != m_selected_tile) {
				m_tile_group->removeTile(mouse_over_tile);
				m_tile_group->addTile(mouse_over_tile);
			}
			else
				m_tile_group->mergeEntries(entry_id, selected_entry_id);
		}
		if(IsKeyDown('M') && mouse_is_up && mouse_over_id != -1 && selected_entry_id != -1) {
			const gfx::Tile *mouse_over_tile = instances[mouse_over_id].tile;
			int entry_id = m_tile_group->findEntry(mouse_over_tile);
			
			m_tile_group->addMatch(selected_entry_id, entry_id, int3(3, 0, 3));
		}

		if(selected_entry_id != -1)	{
			const TileGroup::Entry &entry = (*m_tile_group)[selected_entry_id];
			for(int t = 0; t < (int)entry.m_tiles.size(); t++)
				entry.m_tiles[t]->m_temp |= 1;
			for(int m = 0; m < (int)entry.m_matches.size(); m++)
				(*m_tile_group)[entry.m_matches[m].m_entry_id].representative()->m_temp |= 2;
		}

		//TODO: brać pod uwagę że podczas normalnego rysowania tile będzie przesunięty o boundsy
	}

	//fontTex.Bind();
	//font.SetSize(int2(35, 25));

	//font.SetPos(int2(5, 65));
	//Compare(tiles[cmpId[0]], tiles[cmpId[1]]);
	
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

	if(m_mode == mModify && m_selected_tile) {
		SetScissorRect(bottom_rect);
		int entry_id = m_tile_group->findEntry(m_selected_tile);
		TileGroup::Entry &entry = m_tile_group->m_entries[entry_id];
		const gfx::Tile *tile = entry.representative();

		int2 pos(bottom_rect.min.x + 50, bottom_rect.min.y + bottom_rect.Height() / 2 - tile->GetBounds().Height());
		pos.x += m_offset[2];

		if(m_selected_match_id >= 0 && m_selected_match_id < entry.m_matches.size()) {
			TileGroup::Match &match = entry.m_matches[m_selected_match_id];
		
			struct { int key; int3 offset; } actions[] = {
				{ Key_kp_1, int3( 0, 0,  1) },
				{ Key_kp_2, int3( 1, 0,  1) },
				{ Key_kp_3, int3( 1, 0,  0) },
				{ Key_kp_4, int3(-1, 0,  1) },
				{ Key_kp_6, int3( 1, 0, -1) },
				{ Key_kp_7, int3(-1, 0,  0) },
				{ Key_kp_8, int3(-1, 0, -1) },
				{ Key_kp_9, int3( 0, 0, -1) },
				{ Key_kp_add,		int3(0,  1, 0) },
				{ Key_kp_subtract,	int3(0, -1, 0) } };

			for(int a = 0; a < COUNTOF(actions); a++)
				if(IsKeyDown(actions[a].key)) {
					match.m_offset += actions[a].offset;
					break;
				}
		}

		for(int m = 0; m < (int)entry.m_matches.size(); m++) {
			int2 offset = -tile->GetBounds().min;

			const TileGroup::Entry matched_entry = (*m_tile_group)[entry.m_matches[m].m_entry_id];
			const gfx::Tile *matched_tile = matched_entry.representative();
			const TileGroup::Match match = entry.m_matches[m];
			int2 matched_offset = WorldToScreen(match.m_offset);

			bool draw_first = match.m_offset.z > 0 || (match.m_offset.z >= 0 && match.m_offset.x > 0);
			IRect tile_rect(pos, pos + tile->GetBounds().Size());

			if(!mouse_is_up && IsMouseKeyDown(0) && tile_rect.IsInside(mouse_pos))
				m_selected_match_id = m;

			if(draw_first)
				tile->Draw(pos + offset);
			matched_tile->Draw(pos + offset + matched_offset);
			if(!draw_first)
				tile->Draw(pos + offset);
			if(m_selected_match_id == m) {
				DTexture::Bind0();
				DrawRect(tile_rect, Color(255, 255, 255));
			}

			pos.x += tile->GetBounds().Width() + 100;
		}
	}

	SetScissorTest(false);
}

void TileGroupEditor::setSource(const vector<gfx::Tile> *tiles) {
	m_tiles = tiles;
}

void TileGroupEditor::setTarget(TileGroup* tile_group) {
	m_tile_group = tile_group;
}

int TileGroupEditor::tileCount() const {
	return m_tiles? (int)m_tiles->size() : 0;
}

const gfx::Tile *TileGroupEditor::getTile(int idx) const {
	return m_tiles? &(*m_tiles)[idx] : nullptr;
}

const char *TileGroupEditor::title() const {
	return m_mode == mAddRemove? "TileGroupEditor::adding/removing" : "TileGroupEditor::modifying";
}
