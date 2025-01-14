// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "editor/entities_editor.h"
#include "editor/view.h"
#include "game/entity_map.h"
#include "game/tile.h"
#include "game/tile_map.h"
#include "game/trigger.h"
#include "gfx/drawing.h"
#include "gfx/scene_renderer.h"
#include <algorithm>
#include <set>

using namespace game;

namespace ui {

static const EnumMap<EntitiesEditorMode, const char *> s_mode_desc = {
	{"[S]electing entities", "[P]lacing entities"}};

const char *describe(EntitiesEditorMode mode) { return s_mode_desc[mode]; }

EntitiesEditor::EntitiesEditor(game::TileMap &tile_map, game::EntityMap &entity_map, View &view,
							   IRect rect)
	: ui::Window(rect, ColorId::transparent), m_view(view), m_tile_map(tile_map),
	  m_entity_map(entity_map) {
	m_is_selecting = false;
	m_is_moving = false;

	m_mode = Mode::selecting;
	m_selection = IRect();
	m_cursor_pos = float3(0, 0, 0);
	m_proto_angle = 0;
	m_trigger_mode = 0;
}

void EntitiesEditor::setProto(game::PEntity proto) { m_proto = std::move(proto); }

void EntitiesEditor::onInput(const InputState &state) {
	auto mouse_pos = state.mousePos() - clippedRect().min();
	computeCursor(mouse_pos, mouse_pos, state.isKeyPressed(InputKey::lshift));

	if(state.isKeyDown('s')) {
		m_mode = Mode::selecting;
		sendEvent(this, Event::button_clicked, (int)m_mode);
	}
	if(state.isKeyDown('p')) {
		m_mode = Mode::placing;
		sendEvent(this, Event::button_clicked, (int)m_mode);
	}

	if(m_proto) {
		int inc = 0;
		if(state.isKeyDown(InputKey::left))
			inc = -1;
		if(state.isKeyDown(InputKey::right))
			inc = 1;
		int dir_count = m_proto->sprite().dirCount(0);

		if(inc && dir_count)
			m_proto_angle = (m_proto_angle + inc + dir_count) % dir_count;
		m_proto->setDirAngle(pi * 2.0f * (float)m_proto_angle / float(dir_count));
	}

	if(state.isKeyPressed(InputKey::del)) {
		for(int i = 0; i < (int)m_selected_ids.size(); i++)
			m_entity_map.remove(m_selected_ids[i]);
		m_selected_ids.clear();
	}

	m_view.update(state);
}

void EntitiesEditor::computeCursor(int2 start, int2 end, bool floor_mode) {
	float2 height_off = worldToScreen(float3(0, 0, 0));

	start += m_view.pos();
	end += m_view.pos();

	Ray3F ray = screenRay(start);

	FlagsType flags = Flags::all;
	if(floor_mode)
		flags = flags & ~(Flags::wall_tile | Flags::object_tile);

	auto isect = m_tile_map.trace(ray, -1, flags | Flags::visible);
	float3 pos = isect.first == -1 ? (float3)asXZ(screenToWorld(start)) : ray.at(isect.second);

	m_cursor_pos = (float3)::round(pos);
	m_selection = IRect(vmin(start, end), vmax(start, end));
}

void EntitiesEditor::findVisible(vector<int> &out, const IRect &rect) const {
	out.clear();

	std::set<int> indices;
	vector<int> temp;
	temp.reserve(100);

	static constexpr int block_size = 64;

	for(int gy = m_selection.y(); gy <= m_selection.ey(); gy += block_size)
		for(int gx = m_selection.x(); gx <= m_selection.ex(); gx += block_size) {
			temp.clear();

			IRect block_rect(int2(gx, gy),
							 vmin(int2(gx + block_size, gy + block_size), m_selection.max()));
			m_entity_map.findAll(temp, block_rect, Flags::all | Flags::visible);
			for(int n = 0; n < (int)temp.size(); n++)
				if(indices.find(temp[n]) != indices.end()) {
					temp[n--] = temp.back();
					temp.pop_back();
				}

			for(int y = gy, endy = min(m_selection.ey(), gy + block_size); y <= endy; y += 2)
				for(int x = m_selection.x(), endx = min(gx + block_size, m_selection.ex());
					x <= endx; x += 2) {
					int2 point(x, y);
					if(point.x + 1 <= m_selection.ex() && (y & 1))
						point.x++;

					bool tile_isected = false;
					int tile_idx = -1;

					for(int i = 0; i < (int)temp.size(); i++) {
						const auto &object = m_entity_map[temp[i]];
						bool is_trigger = object.ptr->typeId() == EntityId::trigger;

						if(!is_trigger && !object.ptr->testPixel(point))
							continue;
						if(is_trigger && !object.ptr->currentScreenRect().containsCell(point))
							continue;

						if(!tile_isected) {
							tile_idx = m_tile_map.pixelIntersect(int2(x, y), Flags::walkable_tile |
																				 Flags::visible);
							tile_isected = true;
						}
						if(tile_idx != -1) {
							if(drawingOrder(m_tile_map[tile_idx].bbox,
											m_entity_map[temp[i]].bbox) == 1)
								continue;
						}

						indices.insert(temp[i]);
						temp[i--] = temp.back();
						temp.pop_back();
					}
				}
		}

	out.insert(out.begin(), indices.begin(), indices.end());
}

bool EntitiesEditor::onMouseDrag(const InputState &state, int2 start, int2 current, int key,
								 int is_final) {
	bool shift_pressed = state.isKeyPressed(InputKey::lshift);
	if(key == 0 && !state.isKeyPressed(InputKey::lctrl)) {
		computeCursor(start, current, shift_pressed);
		m_is_selecting = !is_final;

		if(m_mode == Mode::selecting && is_final && is_final != -1) {
			findVisible(m_selected_ids, m_selection);
			computeCursor(current, current, shift_pressed);
		} else if(m_mode == Mode::placing) {
			if(m_proto->typeId() == EntityId::trigger) {
				Trigger *trigger = static_cast<Trigger *>(m_proto.get());

				if(m_trigger_mode == 0) {
					m_trigger_box =
						m_view.computeCursor(start, current, int3(1, 1, 1), m_cursor_pos.y, 0);

					if(state.isMouseButtonDown(InputButton::right)) {
						m_trigger_mode = 1;
						m_trigger_offset = current;
					}
				}

				IBox new_box = m_trigger_box;
				if(m_trigger_mode == 1) {
					int offset = screenToWorld(int2(0, m_trigger_offset.y - current.y)).y;
					if(offset < 0)
						new_box = {{new_box.x(), new_box.y() + offset, new_box.z()}, new_box.max()};
					else
						new_box = {new_box.min(),
								   {new_box.ex(), new_box.ey() + offset, new_box.ez()}};
				}

				trigger->setBox((FBox)new_box);
				m_cursor_pos = trigger->pos();

				if(is_final > 0) {
					m_entity_map.add(PEntity(m_proto->clone()));
				}
				if(is_final) {
					m_trigger_mode = 0;
					trigger->setBox(FBox(0, 0, 0, 1, 1, 1) + trigger->pos());
				}
			} else if(is_final > 0)
				m_entity_map.add(PEntity(m_proto->clone()));
		}

		return true;
	} else if(key == 1 && m_mode == Mode::selecting) {
		if(!m_is_moving) {
			m_is_moving_vertically = state.isKeyPressed(InputKey::lshift);
			m_is_moving = true;
		}

		if(m_is_moving_vertically)
			m_move_offset = int3(0, screenToWorld(int2(0, start.y - current.y)).y, 0);
		else
			m_move_offset = asXZY(screenToWorld(current - start), 0);

		if(is_final)
			m_is_moving = false;

		if(is_final > 0) {
			for(int n = 0; n < (int)m_selected_ids.size(); n++) {
				auto &object = m_entity_map[m_selected_ids[n]];
				object.ptr->setPos(object.ptr->pos() + float3(m_move_offset));
				m_entity_map.update(m_selected_ids[n]);
			}
		}

		return true;
	}

	return false;
}

void EntitiesEditor::drawBoxHelpers(Canvas2D &out, const IBox &box) const {
	int3 pos = box.min(), bbox = box.max() - box.min();
	int3 tsize = asXZY(m_tile_map.dimensions(), 32);

	drawLine(out, int3(0, pos.y, pos.z), int3(tsize.x, pos.y, pos.z), Color(0, 255, 0, 127));
	drawLine(out, int3(0, pos.y, pos.z + bbox.z), int3(tsize.x, pos.y, pos.z + bbox.z),
			 Color(0, 255, 0, 127));

	drawLine(out, int3(pos.x, pos.y, 0), int3(pos.x, pos.y, tsize.z), Color(0, 255, 0, 127));
	drawLine(out, int3(pos.x + bbox.x, pos.y, 0), int3(pos.x + bbox.x, pos.y, tsize.z),
			 Color(0, 255, 0, 127));

	int3 tpos(pos.x, 0, pos.z);
	drawBBox(out, IBox(tpos, tpos + int3(bbox.x, pos.y, bbox.z)), Color(0, 0, 255, 127));

	drawLine(out, int3(0, 0, pos.z), int3(tsize.x, 0, pos.z), Color(0, 0, 255, 127));
	drawLine(out, int3(0, 0, pos.z + bbox.z), int3(tsize.x, 0, pos.z + bbox.z),
			 Color(0, 0, 255, 127));

	drawLine(out, int3(pos.x, 0, 0), int3(pos.x, 0, tsize.z), Color(0, 0, 255, 127));
	drawLine(out, int3(pos.x + bbox.x, 0, 0), int3(pos.x + bbox.x, 0, tsize.z),
			 Color(0, 0, 255, 127));
}

FBox EntitiesEditor::computeOvergroundBox(const FBox &bbox) const {
	int over_ground = 0;
	FBox test_box = {bbox.min(), {bbox.ex(), bbox.y() + 1.0f, bbox.z()}};

	while(over_ground < 16 && m_tile_map.findAny(test_box - float3(0, 1 + over_ground, 0)) == -1)
		over_ground++;

	if(over_ground)
		return FBox(asXZY(bbox.min().xz(), bbox.y() - over_ground),
					asXZY(bbox.max().xz(), bbox.y()));
	return FBox();
}

void EntitiesEditor::drawContents(Canvas2D &out) const {
	m_view.updateVisibility();
	SceneRenderer renderer(clippedRect(), m_view.pos());

	{
		vector<int> visible_ids;
		visible_ids.reserve(1024);
		m_tile_map.findAll(visible_ids, renderer.targetRect(), Flags::all | Flags::visible);

		for(int i = 0; i < (int)visible_ids.size(); i++) {
			auto &object = m_tile_map[visible_ids[i]];
			int3 pos(object.bbox.min());
			object.ptr->addToRender(renderer, pos, ColorId::white);
		}

		visible_ids.clear();
		m_entity_map.findAll(visible_ids, renderer.targetRect(), Flags::all | Flags::visible);

		vector<char> selection_map(m_entity_map.size(), 0);
		vector<float3> old_positions(m_selected_ids.size());

		for(int n = 0; n < (int)m_selected_ids.size(); n++) {
			selection_map[m_selected_ids[n]] = 1;
			visible_ids.push_back(m_selected_ids[n]);

			if(m_is_moving) {
				auto &object = m_entity_map[m_selected_ids[n]];
				old_positions[n] = object.ptr->pos();
				object.ptr->setPos(old_positions[n] + float3(m_move_offset));
				m_entity_map.update(m_selected_ids[n]);
			}
		}

		std::sort(visible_ids.begin(), visible_ids.end());
		visible_ids.resize(std::unique(visible_ids.begin(), visible_ids.end()) -
						   visible_ids.begin());

		for(int n = 0; n < (int)visible_ids.size(); n++) {
			auto &object = m_entity_map[visible_ids[n]];
			float3 old_pos = object.ptr->pos();
			bool is_selected = selection_map[visible_ids[n]];

			FBox bbox = object.ptr->boundingBox();

			if(object.ptr->typeId() == EntityId::trigger) {
				renderer.addBox(bbox, Color(ColorId::green, 64), true);
				renderer.addBox(
					FBox(bbox.min() + float3(0.1, 0.1, 0.1), bbox.max() - float3(0.1, 0.1, 0.1)),
					ColorId::green);
			} else {
				object.ptr->addToRender(renderer);
			}

			bool is_colliding =
				m_tile_map.findAny(bbox) != -1 || m_entity_map.findAny(bbox, visible_ids[n]) != -1;
			if(is_colliding)
				renderer.addBox(bbox, ColorId::red);
			if(is_selected) {
				if(!is_colliding)
					renderer.addBox(bbox, ColorId::white);
				FBox overground_box = computeOvergroundBox(bbox);
				if(!overground_box.empty())
					renderer.addBox(overground_box, ColorId::yellow);
			}
		}

		if(m_is_moving)
			for(int n = 0; n < (int)m_selected_ids.size(); n++) {
				auto &object = m_entity_map[m_selected_ids[n]];
				object.ptr->setPos(old_positions[n]);
				m_entity_map.update(m_selected_ids[n]);
			}
	}

	if(m_proto && m_mode == Mode::placing) {
		m_proto->setPos(m_cursor_pos);
		m_proto->addToRender(renderer);
		FBox bbox = m_proto->boundingBox();

		bool is_colliding = m_tile_map.findAny(bbox) != -1 || m_entity_map.findAny(bbox) != -1;
		renderer.addBox(bbox, is_colliding ? ColorId::red : ColorId::white);
		if(bbox.ey() == bbox.y())
			bbox = bbox.enlarge({}, {0.0f, 1.0f, 0.0f});

		FBox overground_box = computeOvergroundBox(bbox);
		if(!overground_box.empty())
			renderer.addBox(overground_box, ColorId::yellow);
	}

	renderer.render(out);
	if(m_mode == Mode::selecting && m_is_selecting)
		out.addRect(m_selection - m_view.pos(), ColorId::white);

	out.setScissorRect(clippedRect());
	out.setViewPos(-clippedRect().min() + m_view.pos());
	m_view.drawGrid(out);

	out.setViewPos(-clippedRect().min());
	auto &font = res::getFont(WindowStyle::fonts[1]);

	font.draw(out, float2(0, clippedRect().height() - 25), {ColorId::white, ColorId::black},
			  stdFormat("Cursor: (%.0f, %.0f, %.0f)  Grid: %d Mode: %s\n", m_cursor_pos.x,
						m_cursor_pos.y, m_cursor_pos.z, m_view.gridHeight(), s_mode_desc[m_mode]));
}

}
