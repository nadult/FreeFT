#include "grid.h"
#include "sys/profiler.h"

//TODO: better names, refactoring, remove copy&pasted code in intersection functions 
Grid::Node::Node()
	:size(0), first_id(-1), first_overlap_id(-1), is_dirty(false),
	bbox(FBox::empty()), rect(IRect::empty()), obj_flags(0) { }
	
	
bool Grid::flagTest(int object, int test) {
	return (object & type_flags) && ((object & functional_flags & test) == (test & functional_flags));
}

Grid::Grid(const int2 &size) {
	m_bounding_box = FBox::empty();
	m_size = worldToGrid(size + int2(node_size - 1, node_size - 1));
	if(m_size.x * m_size.y > 0) {
		m_nodes.resize(m_size.x * m_size.y);
		m_row_rects.resize(m_size.y, int2(0, 0));
		m_free_list.reserve(1024);
		m_free_overlaps.reserve(1024);
		m_overlaps.reserve(1024 * 16);
		m_disabled_overlaps.reserve(256);
	}
}
	
int Grid::add(const ObjectDef &def) {
	m_bounding_box = m_bounding_box.isEmpty()? def.bbox : sum(m_bounding_box, def.bbox);
	IRect grid_box = nodeCoords(def.bbox);

	for(int y = grid_box.min.y; y <= grid_box.max.y; y++) {
		int2 &min_max = m_row_rects[y];
		min_max = min_max.y == min_max.x?
				int2(def.rect_pos.y, def.rect_pos.y + def.rect_size.y) :
				int2(min(min_max.x, def.rect_pos.y), max(min_max.y, def.rect_pos.y + def.rect_size.y));
	}

	int object_id;
	if(m_free_list.empty()) {
		m_objects.push_back(Object());
		object_id = (int)m_objects.size() - 1;
		m_objects.back().is_disabled = 0;
	}
	else {
		object_id = m_free_list.back();
		m_free_list.pop_back();
	}
	
	Object &object = m_objects[object_id];
	((ObjectDef&)object) = def;

	if(grid_box.min == grid_box.max) {
		int node_id = nodeAt(grid_box.min);
		Node &node = m_nodes[node_id];

		object.prev_id = -1;
		object.node_id = node_id;
		link(object_id, node.first_id);

		updateNode(node_id, def);
		node.size++;
		node.first_id = object_id;
	}
	else {
		object.node_id = object.prev_id = object.next_id = -1;
		for(int y = grid_box.min.y; y <= grid_box.max.y; y++)
			for(int x = grid_box.min.x; x <= grid_box.max.x; x++) {
				int node_id = nodeAt(int2(x, y));
				Node &node = m_nodes[node_id];
				updateNode(node_id, def);
				node.size++;

				Overlap overlap;
				overlap.next_id = node.first_overlap_id;
				overlap.object_id = object_id;

				if(m_free_overlaps.empty()) {
					node.first_overlap_id = (int)m_overlaps.size();
					m_overlaps.push_back(overlap);
				}
				else {
					m_overlaps[node.first_overlap_id = m_free_overlaps.back()] = overlap;
					m_free_overlaps.pop_back();
				}
			}
	}

	return object_id;
}

void Grid::remove(int idx) {
	Object &object = m_objects[idx];
	DASSERT(object.ptr != nullptr);

	if(object.node_id == -1) {
		IRect grid_box = nodeCoords(object.bbox);
		for(int y = grid_box.min.y; y <= grid_box.max.y; y++)
			for(int x = grid_box.min.x; x <= grid_box.max.x; x++) {
				int node_id = nodeAt(int2(x, y));
				Node &node = m_nodes[node_id];
				node.size--;
				node.is_dirty = true;
				int overlap_id = node.first_overlap_id;
				int prev_id = -1;

				while(true) {
					DASSERT(overlap_id != -1);
					Overlap &overlap = m_overlaps[overlap_id];
					if(overlap.object_id == idx) {
						m_free_overlaps.push_back(overlap_id);
						if(prev_id != -1)
							m_overlaps[prev_id].next_id = overlap.next_id;
						if(node.first_overlap_id == overlap_id)
							node.first_overlap_id = overlap.next_id;
						break;
					}
					prev_id = overlap_id;
					overlap_id = m_overlaps[overlap_id].next_id;
				}
			}
	}
	else {
		Node &node = m_nodes[object.node_id];
		if(node.first_id == idx)
			node.first_id = object.next_id;
		node.size--;
		node.is_dirty = true;
		link(object.prev_id, object.next_id);
		object.node_id = -1;
	}

	object.ptr = nullptr;
	m_free_list.push_back(idx);	
}

void Grid::update(int id, const ObjectDef &object) {
	//TODO: speed up
	remove(id);
	int new_id = add(object);
	DASSERT(new_id == id);
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

	node.bbox = sum(def.bbox, node.bbox);
	node.rect = sum(def.rect(), node.rect);
	node.obj_flags |= def.flags;
}

void Grid::updateNode(int id) const {
	const Node &node = m_nodes[id];

	if(node.size != 0) {
		int cur_id = node.first_id;
		while(cur_id != -1) {
			const Object &obj = m_objects[cur_id];
			updateNode(id, obj);
			cur_id = obj.next_id;
		}
		int overlap_id = node.first_overlap_id;
		while(overlap_id != -1) {
			const Overlap &overlap = m_overlaps[overlap_id];
			updateNode(id, m_objects[overlap.object_id]);
			overlap_id = overlap.next_id;
		}
	}
	else {
		node.bbox = FBox::empty();
		node.rect = IRect::empty();
		node.obj_flags = 0;
	}

	node.is_dirty = false;
}

void Grid::link(int cur_id, int next_id) {
	if(cur_id != -1)
		m_objects[cur_id ].next_id = next_id;
	if(next_id != -1)
		m_objects[next_id].prev_id = cur_id;
}

const IRect Grid::nodeCoords(const FBox &box) const {
	return IRect(	worldToGrid( max(int2(0, 0), int2(box.min.x, box.min.z)) ),
					min(m_size - int2(1, 1),
						worldToGrid( int2(box.max.x - constant::epsilon, box.max.z - constant::epsilon) )) );
}

bool Grid::isInside(const float3 &pos) const {
	return pos.x >= 0 && pos.z >= 0 && pos.x < m_size.x * node_size && pos.z < m_size.y * node_size;
}

bool Grid::isInside(const FBox &box) const {
	return box.min.x >= 0 && box.min.z >= 0 && box.max.x <= m_size.x * node_size && box.max.z <= m_size.y * node_size;
}

int Grid::extractObjects(int node_id, const Object **out, int ignored_id, int flags) const {
	const Object **start = out;

	const Node &node = m_nodes[node_id];
	int object_id = node.first_id;
	while(object_id != -1) {
		const Object &object = m_objects[object_id];
		if(flagTest(object.flags, flags) && object_id != ignored_id)
			*out++ = &object;
		object_id = object.next_id;
	}

	int overlap_id = node.first_overlap_id;
	while(overlap_id != -1) {
		object_id = m_overlaps[overlap_id].object_id;
		const Object &object = m_objects[object_id];
		if(flagTest(object.flags, flags) && object_id != ignored_id && !object.is_disabled)
			*out++ = &object;
		overlap_id = m_overlaps[overlap_id].next_id;
	}

	return out - start;
}

void Grid::printInfo() const {
	printf("Grid(%d, %d):\n", m_size.x, m_size.y);
	printf("       nodes(%d): %.2f KB\n", (int)m_nodes.size(), (float)m_nodes.size() * sizeof(Node) / 1024.0);
	printf("     objects(%d): %.2f KB\n", (int)m_objects.size(), (float)m_objects.size() * sizeof(Object) / 1024.0);
	printf("    overlaps(%d): %.2f KB\n", (int)m_overlaps.size(), (float)m_overlaps.size() * sizeof(Overlap) / 1024.0);
	printf("  free_lists(%d): %.2f KB\n", (int)m_free_list.size() + (int)m_free_overlaps.size(),
										(float)(m_free_list.size() + m_free_overlaps.size()) * sizeof(int) / 1024.0);
	printf("  sizeof(Node): %d\n", (int)sizeof(Node));
	printf("  sizeof(Object): %d\n", (int)sizeof(Object));
}

void Grid::swap(Grid &rhs) {
	std::swap(m_bounding_box, rhs.m_bounding_box);
	std::swap(m_size, rhs.m_size);
	m_row_rects.swap(rhs.m_row_rects);
	m_free_list.swap(rhs.m_free_list);
	m_free_overlaps.swap(rhs.m_free_overlaps);
	m_overlaps.swap(rhs.m_overlaps);
	m_nodes.swap(rhs.m_nodes);
	m_objects.swap(rhs.m_objects);
}

void Grid::clear() {
	Grid empty(dimensions());
	swap(empty);
}

void Grid::disableOverlap(const Object *object) const {
	DASSERT(object && object->node_id == -1);
	object->is_disabled = 1;
	m_disabled_overlaps.push_back(object - m_objects.data());
}

void Grid::clearDisables() const {
	if(!m_disabled_overlaps.empty()) {
		for(int n = 0; n < (int)m_disabled_overlaps.size(); n++)
			m_objects[m_disabled_overlaps[n]].is_disabled = 0;
		m_disabled_overlaps.clear();
	}
}
