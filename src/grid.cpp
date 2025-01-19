// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "grid.h"

#define INSERT(list, id)                                                                           \
	listInsert([&](int idx) -> ListNode & { return m_objects[idx].node; }, list, id)
#define REMOVE(list, id)                                                                           \
	listRemove([&](int idx) -> ListNode & { return m_objects[idx].node; }, list, id)

#define OV_INSERT(list, id)                                                                        \
	listInsert([&](int idx) -> ListNode & { return m_overlaps[idx].node; }, list, id)
#define OV_REMOVE(list, id)                                                                        \
	listRemove([&](int idx) -> ListNode & { return m_overlaps[idx].node; }, list, id)

//TODO: better names, refactoring, remove copy&pasted code in intersection functions
Grid::Node::Node() : size(0), is_dirty(false), bbox(FBox()), rect(IRect()), obj_flags(0) {}

Grid::Grid(const int2 &size) {
	m_bounding_box = FBox();
	m_size = worldToGrid(size + int2(node_size - 1, node_size - 1));
	if(m_size.x * m_size.y > 0) {
		m_nodes.resize(m_size.x * m_size.y);
		m_row_rects.resize(m_size.y + 1, int2(0, 0));
		//TODO: row_rects not updated when removing objects, this will degrade performance
		// when drawing entitiy grids

		m_overlaps.reserve(1024 * 16);
		m_disabled_overlaps.reserve(256);
	}
}

int Grid::findFreeObject() {
	if(m_free_objects.empty()) {
		m_objects.emplace_back(Object());
		INSERT(m_free_objects, (int)m_objects.size() - 1);
	}

	return m_free_objects.head;
}

int Grid::findFreeOverlap() {
	if(m_free_overlaps.empty()) {
		m_overlaps.emplace_back(Overlap());
		OV_INSERT(m_free_overlaps, (int)m_overlaps.size() - 1);
	}

	return m_free_overlaps.head;
}

void Grid::add(int object_id, const ObjectDef &def) {
	DASSERT(object_id >= 0);
	if(object_id >= (int)m_objects.size()) {
		int old_size = (int)m_objects.size();
		m_objects.resize(object_id + 1);
		for(int n = old_size; n < (int)m_objects.size(); n++)
			INSERT(m_free_objects, n);
	}

	REMOVE(m_free_objects, object_id);

	m_bounding_box = m_bounding_box.empty() ? def.bbox : enclose(m_bounding_box, def.bbox);
	IRect grid_box = nodeCoords(def.bbox);

	for(int y = grid_box.y(); y <= grid_box.ey(); y++) {
		int2 &min_max = m_row_rects[y];
		min_max = min_max.y == min_max.x ? int2(def.rect_pos.y, def.rect_pos.y + def.rect_size.y) :
										   int2(min(min_max.x, def.rect_pos.y),
												max(min_max.y, def.rect_pos.y + def.rect_size.y));
	}

	Object &object = m_objects[object_id];
	DASSERT(object.ptr == nullptr);
	((ObjectDef &)object) = def;

	if(grid_box.min() == grid_box.max()) {
		int node_id = nodeAt(grid_box.min());
		Node &node = m_nodes[node_id];

		INSERT(node.object_list, object_id);
		object.node_id = node_id;

		updateNode(node_id, def);
		node.size++;
	} else {
		for(int y = grid_box.y(); y <= grid_box.ey(); y++)
			for(int x = grid_box.x(); x <= grid_box.ex(); x++) {
				int node_id = nodeAt(int2(x, y));
				Node &node = m_nodes[node_id];
				updateNode(node_id, def);
				node.size++;

				int overlap_id = findFreeOverlap();

				Overlap &overlap = m_overlaps[overlap_id];
				overlap.object_id = object_id;
				OV_REMOVE(m_free_overlaps, overlap_id);
				OV_INSERT(node.overlap_list, overlap_id);
			}
	}
}

void Grid::remove(int idx) {
	Object &object = m_objects[idx];
	if(object.ptr == nullptr)
		return;

	if(object.node_id == -1) {
		IRect grid_box = nodeCoords(object.bbox);
		for(int y = grid_box.y(); y <= grid_box.ey(); y++)
			for(int x = grid_box.x(); x <= grid_box.ex(); x++) {
				Node &node = m_nodes[nodeAt(int2(x, y))];
				node.size--;
				node.is_dirty = true;
				int overlap_id = node.overlap_list.head;

				while(true) {
					DASSERT(overlap_id != -1);
					Overlap &overlap = m_overlaps[overlap_id];

					if(overlap.object_id == idx) {
						OV_REMOVE(node.overlap_list, overlap_id);
						OV_INSERT(m_free_overlaps, overlap_id);
						break;
					}

					overlap_id = overlap.node.next;
				}
			}
	} else {
		Node &node = m_nodes[object.node_id];
		REMOVE(node.object_list, idx);
		object.node_id = -1;
		node.size--;
		node.is_dirty = true;
	}

	object.ptr = nullptr;
	INSERT(m_free_objects, idx);
}

bool Grid::update(int idx, const ObjectDef &object) {
	DASSERT(idx >= 0 && idx < size());
	const ObjectDef &old_object = (*this)[idx];
	DASSERT(object.ptr == old_object.ptr);

	if(old_object.bbox != object.bbox || old_object.rect_pos != object.rect_pos ||
	   old_object.rect_size != object.rect_size || old_object.flags != object.flags) {
		//TODO: speed up; in most cases it can be done fast, coz we have a pointer to node
		remove(idx);
		add(idx, object);
		return true;
	}

	return false;
}

void Grid::updateNodes() {
	for(int n = 0; n < (int)m_nodes.size(); n++)
		updateNode(n);
}

void Grid::updateNode(int node_id, const ObjectDef &def) const {
	const Node &node = m_nodes[node_id];

	if(node.size == 0) {
		node.bbox = def.bbox;
		node.rect = def.rect();
		node.obj_flags = def.flags;
		return;
	}

	node.bbox = enclose(def.bbox, node.bbox);
	node.rect = enclose(def.rect(), node.rect);
	node.obj_flags |= def.flags;
}

void Grid::updateNode(int id) const {
	const Node &node = m_nodes[id];

	if(node.size != 0) {
		int cur_id = node.object_list.head;
		while(cur_id != -1) {
			const Object &obj = m_objects[cur_id];
			updateNode(id, obj);
			cur_id = obj.node.next;
		}
		int overlap_id = node.overlap_list.head;
		while(overlap_id != -1) {
			const Overlap &overlap = m_overlaps[overlap_id];
			updateNode(id, m_objects[overlap.object_id]);
			overlap_id = overlap.node.next;
		}
	} else {
		node.bbox = FBox();
		node.rect = IRect();
		node.obj_flags = 0;
	}

	node.is_dirty = false;
}

const IRect Grid::nodeCoords(const FBox &box) const {
	auto pmin = worldToGrid(vmax(int2(0, 0), int2(box.x(), box.z())));
	auto pmax = worldToGrid(int2(box.ex() - big_epsilon, box.ez() - big_epsilon));
	pmax = vmin(vmax(pmin, pmax), m_size - int2(1, 1));
	return IRect(pmin, pmax);
}

bool Grid::isInside(const float3 &pos) const {
	return pos.x >= 0 && pos.z >= 0 && pos.x < m_size.x * node_size && pos.z < m_size.y * node_size;
}

bool Grid::isInside(const FBox &box) const {
	return box.x() >= 0 && box.z() >= 0 && box.ex() <= m_size.x * node_size &&
		   box.ez() <= m_size.y * node_size;
}

int Grid::extractObjects(int node_id, const Object **out, int ignored_id, int flags) const {
	const Object **start = out;

	const Node &node = m_nodes[node_id];
	int object_id = node.object_list.head;
	while(object_id != -1) {
		const Object &object = m_objects[object_id];
		if(flagTest(object.flags, flags) && object_id != ignored_id)
			*out++ = &object;
		object_id = object.node.next;
	}

	int overlap_id = node.overlap_list.head;
	while(overlap_id != -1) {
		object_id = m_overlaps[overlap_id].object_id;
		const Object &object = m_objects[object_id];
		if(flagTest(object.flags, flags) && object_id != ignored_id && !object.is_disabled)
			*out++ = &object;
		overlap_id = m_overlaps[overlap_id].node.next;
	}

	return out - start;
}

void Grid::printInfo() const {
	printf("Grid(%d, %d):\n", m_size.x, m_size.y);
	printf("       nodes(%d): %.2f KB\n", (int)m_nodes.size(),
		   (float)m_nodes.size() * sizeof(Node) / 1024.0);
	printf("     objects(%d): %.2f KB\n", (int)m_objects.size(),
		   (float)m_objects.size() * sizeof(Object) / 1024.0);
	printf("    overlaps(%d): %.2f KB\n", (int)m_overlaps.size(),
		   (float)m_overlaps.size() * sizeof(Overlap) / 1024.0);
	printf("  sizeof(Node): %d\n", (int)sizeof(Node));
	printf("  sizeof(Object): %d\n", (int)sizeof(Object));
}

void Grid::swap(Grid &rhs) {
	std::swap(m_bounding_box, rhs.m_bounding_box);
	std::swap(m_size, rhs.m_size);

	m_row_rects.swap(rhs.m_row_rects);
	m_overlaps.swap(rhs.m_overlaps);
	m_nodes.swap(rhs.m_nodes);
	m_objects.swap(rhs.m_objects);

	std::swap(m_free_objects, rhs.m_free_objects);
	std::swap(m_free_overlaps, rhs.m_free_overlaps);

	m_disabled_overlaps.swap(rhs.m_disabled_overlaps);
}

void Grid::clear() {
	Grid empty(dimensions());
	swap(empty);
}

void Grid::disableOverlap(const Object *object) const {
	DASSERT(object && object->node_id == -1);
	object->is_disabled = 1;
	m_disabled_overlaps.emplace_back((int)(object - m_objects.data()));
}

void Grid::clearDisables() const {
	if(!m_disabled_overlaps.empty()) {
		for(int n = 0; n < (int)m_disabled_overlaps.size(); n++)
			m_objects[m_disabled_overlaps[n]].is_disabled = 0;
		m_disabled_overlaps.clear();
	}
}
