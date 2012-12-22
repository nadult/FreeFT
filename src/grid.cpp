#include "grid.h"
#include <stddef.h>

//TODO: better names, refactoring, remove copy&pasted code in intersection functions 
Grid::Node::Node()
	:first_id(-1), is_dirty(false), bbox(FBox::empty()), rect(IRect::empty()), obj_flags(0) { }

Grid::Grid(const int2 &size) {
	m_bounding_box = FBox::empty();
	m_size = worldToGrid(size);
	m_nodes.resize(m_size.x * m_size.y);
	m_row_rects.resize(m_size.y, int2(0, 0));

	m_max_bbox_size = int2(1, 1);
	m_free_list.reserve(1024);
}
	
int Grid::add(const Object &obj) {
	m_bounding_box = m_bounding_box.isEmpty()? obj.bbox : sum(m_bounding_box, obj.bbox);
	m_max_bbox_size = max(m_max_bbox_size, int2(obj.bbox.width() + 1.0f, obj.bbox.height() + 1.0f));
	DASSERT(isInside(obj.bbox.min));

	int2 xz_pos = worldToGrid(int2(obj.bbox.min.x, obj.bbox.min.z));
	int node_id = nodeAt(xz_pos);
	Node &node = m_nodes[node_id];

	int2 &row_rect = m_row_rects[xz_pos.y];
	row_rect = row_rect.y == row_rect.x? int2(obj.rect.min.y, obj.rect.max.y)
		: int2(min(row_rect.x, obj.rect.min.y), max(row_rect.y, obj.rect.max.y));

	int inst_id;
	if(m_free_list.empty()) {
		m_instances.push_back(Instance());
		inst_id = (int)m_instances.size() - 1;
	}
	else {
		inst_id = m_free_list.back();
		m_free_list.pop_back();
	}

	Instance &inst = m_instances[inst_id];
	((Object&)inst) = obj;

	inst.prev_id = -1;
	inst.node_id = node_id;
	link(inst_id, node.first_id);

	node.bbox = node.first_id == -1? obj.bbox : sum(obj.bbox, node.bbox);
	node.rect = node.first_id == -1? obj.rect : sum(obj.rect, node.rect);
	node.obj_flags |= obj.flags;
	node.first_id = inst_id;

	return inst_id;
}

void Grid::remove(int idx) {
	Instance &instance = m_instances[idx];
	Node &node = m_nodes[instance.node_id];
	if(node.first_id == idx)
		node.first_id = instance.next_id;
	node.is_dirty = true;
	link(instance.prev_id, instance.next_id);
	instance.ptr = nullptr;
	instance.node_id = -1;
	m_free_list.push_back(idx);	
}

void Grid::update(int id, const Object &object) {
	//TODO: speed up
	remove(id);
	int new_id = add(object);
	DASSERT(new_id == id);
}

void Grid::updateNode(int id) const {
	const Node &node = m_nodes[id];

	if(node.first_id != -1) {
		const Instance *obj = &m_instances[node.first_id];
		node.bbox = obj->bbox;
		node.rect = obj->rect;
		node.obj_flags = obj->flags;
		int cur_id = obj->next_id;

		while(cur_id != -1) {
			obj = &m_instances[cur_id];
			node.bbox = sum(node.bbox, obj->bbox);
			node.rect = sum(node.rect, obj->rect);
			node.obj_flags |= obj->flags;
			cur_id = obj->next_id;
		}
	}
	else {
		node.bbox = FBox::empty();
		node.rect = IRect::empty();
	}

	node.is_dirty = false;
}

void Grid::link(int cur_id, int next_id) {
	if(cur_id != -1)
		m_instances[cur_id ].next_id = next_id;
	if(next_id != -1)
		m_instances[next_id].prev_id = cur_id;
}

const IRect Grid::nodeCoords(const FBox &box) const {
	IRect grid_box((int2)box.min.xz() - m_max_bbox_size, (int2)box.max.xz() + int2(1, 1));
	grid_box.min = worldToGrid(max(grid_box.min, int2(0, 0)));
	grid_box.max = min(worldToGrid(grid_box.max) + int2(1, 1), m_size);
	return grid_box;
}
template <class NodeTest, class InstanceTest, class InstanceAction>
void Grid::iterate(IRect grid_box, NodeTest node_test, InstanceTest instance_test, InstanceAction action) const {
	for(int y = grid_box.min.y; y < grid_box.max.y; y++)
		for(int x = grid_box.min.x; x < grid_box.max.x; x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];

			if(!node_test(node))
				continue;
			bool anything_found = false;

			int instance_id = node.first_id;
			while(instance_id != -1) {
				const Instance &instance = m_instances[instance_id];
				if(instance_test(instance)) {
					action(instance);
					anything_found = true;
				}
				instance_id = instance.next_id;
			}

			if(!anything_found && node.is_dirty)
				updateNode(node_id);	
		}
}


int Grid::findAny(const FBox &box, int ignored_id, int flags) const {
	vector<int> temp;
	findAll(temp, box, ignored_id, flags);
	return temp.empty()? -1 : temp.front();
}
	
void Grid::findAll(vector<int> &out, const FBox &box, int ignored_id, int flags) const {
	IRect grid_box = nodeCoords(box);

	for(int y = grid_box.min.y; y < grid_box.max.y; y++)
		for(int x = grid_box.min.x; x < grid_box.max.x; x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];

			if(!(node.obj_flags & flags) || !areOverlapping(box, node.bbox))
				continue;
			bool anything_found = false;

			int instance_id = node.first_id;
			while(instance_id != -1) {
				const Instance &instance = m_instances[instance_id];
				if((flags & instance.flags) && instance_id != ignored_id)
					if(areOverlapping(box, instance.bbox)) {
						out.push_back(instance_id);
						anything_found = true;
					}
				instance_id = instance.next_id;
			}

			if(!anything_found && node.is_dirty)
				updateNode(node_id);	
		}
}

pair<int, float> Grid::trace(const Segment &segment, int ignored_id, int flags) const {
	float tmin = intersection(segment, m_bounding_box);
	float tmax = -intersection(-segment, m_bounding_box);
			
	float3 p1 = segment.at(max(tmin, segment.min)), p2 = segment.at(min(tmax, segment.max));
	IRect grid_box = nodeCoords(FBox(min(p1, p2), max(p1, p2)));

	int out = -1;
	float out_dist = constant::inf;

	//TODO: speed up
	//TODO: proper order
	for(int y = grid_box.min.y; y < grid_box.max.y; y++)
		for(int x = grid_box.min.x; x < grid_box.max.x; x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];
			if(node.is_dirty)
				updateNode(node_id);

			if(!(node.obj_flags & flags))
				continue;

			float node_dist = intersection(segment, node.bbox);
			if(node_dist >= out_dist)
				continue;

			int instance_id = node.first_id;
			while(instance_id != -1) {
				const Instance &instance = m_instances[instance_id];
				if((flags & instance.flags) && instance_id != ignored_id) {
					float dist = intersection(segment, instance.bbox);
					if(dist < out_dist) {
						out_dist = dist;
						out = instance_id;
					}
				}
				instance_id = instance.next_id;
			}	
		}

	return make_pair(out, out_dist);
}

void Grid::findAll(vector<int> &out, const IRect &view_rect) const {
	IRect grid_box(0, 0, m_size.x, m_size.y);

	for(int y = grid_box.min.y; y < grid_box.max.y; y++) {
		const int2 &row_rect = m_row_rects[y];
		if(row_rect.x >= view_rect.max.y || row_rect.y <= view_rect.min.y)
			continue;
		
		for(int x = grid_box.min.x; x < grid_box.max.x; x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];

			if(!areOverlapping(view_rect, node.rect))
				continue;
			bool anything_found = false;

			int instance_id = node.first_id;
			while(instance_id != -1) {
				const Instance &instance = m_instances[instance_id];
				if(areOverlapping(view_rect, instance.rect)) {
					out.push_back(instance_id);
					anything_found = true;
				}
				instance_id = instance.next_id;
			}

			if(!anything_found && node.is_dirty)
				updateNode(node_id);	
		}
	}
}

int Grid::pixelIntersect(const int2 &screen_pos, bool (*pixelTest)(const Object&, const int2&)) const {
	IRect grid_box(0, 0, m_size.x, m_size.y);
	
	int best = -1;
	FBox best_box;

	for(int y = grid_box.min.y; y < grid_box.max.y; y++) {
		const int2 &row_rect = m_row_rects[y];
		if(row_rect.x >= screen_pos.y || row_rect.y <= screen_pos.y)
			continue;

		for(int x = grid_box.min.x; x < grid_box.max.x; x++) {
			int node_id = nodeAt(int2(x, y));
			const Node &node = m_nodes[node_id];

			if(!node.rect.isInside(screen_pos))
				continue;
			if(node.is_dirty)
				updateNode(node_id);	

			int instance_id = node.first_id;
			while(instance_id != -1) {
				const Instance &instance = m_instances[instance_id];
				if(instance.rect.isInside(screen_pos) && pixelTest(instance, screen_pos)) {
					if(best == -1 || drawingOrder(instance.bbox, best_box) == 1) {
						best = instance_id;
						best_box = instance.bbox;
					}
				}
				instance_id = instance.next_id;
			}
		}
	}

	return best;
}

bool Grid::isInside(const float3 &pos) const {
	return pos.x >= 0 && pos.z >= 0 && pos.x < m_size.x * node_size && pos.z < m_size.y * node_size;
}

bool Grid::isInside(const FBox &box) const {
	return box.min.x >= 0 && box.min.z >= 0 && box.max.x < m_size.x * node_size && box.max.z < m_size.y * node_size;
}

void Grid::printInfo() const {
	printf("Grid(%d, %d):\n", m_size.x, m_size.y);
	printf("  nodes(%d): %.0f KB\n", (int)m_nodes.size(), (float)m_nodes.size() * sizeof(Node) / 1024.0);
	printf("  instances(%d): %.0f KB\n", (int)m_instances.size(), (float)m_instances.size() * sizeof(Instance) / 1024.0);
	printf("  sizeof(Node): %d\n", sizeof(Node));
	printf("  sizeof(Instance): %d\n", sizeof(Instance));
}
