#ifndef GRID_H
#define GRID_H

#include "base.h"

// When accessing objects directly, you have to check ptr for null,
// because some of the objects may be invalid; you don't have to check
// if you're accesing object at index returned from one of the interection
// functions.
class Grid {
public:
	enum {
		node_size = 24
	};

	struct Object {
		Object(const void *ptr = nullptr, const FBox &bbox = FBox::empty(), const IRect &rect = IRect::empty(), int flags = -1)
			:ptr(ptr), bbox(bbox), rect(rect), flags(flags) { }

		const void *ptr; 
		FBox bbox;
		IRect rect;
		int flags;
	};

	Grid(const int2 &dimensions = int2(0, 0));

	int add(const Object&);
	void remove(int idx);
	void update(int idx, const Object&);

	int findAny(const FBox &box, int ignored_id = -1, int flags = -1) const;
	void findAll(vector<int> &out, const FBox &box, int ignored_id = -1, int flags = -1) const;

	pair<int, float> trace(const Segment &segment, int ignored_id = -1, int flags = -1) const;
	
	void findAll(vector<int> &out, const IRect &view_rect) const;
	int pixelIntersect(const int2 &pos, bool (*pixelTest)(const Object&, const int2 &pos)) const;

	int size() const { return (int)m_instances.size(); }
	const Object &operator[](int idx) const { return m_instances[idx]; }
	Object &operator[](int idx) { return m_instances[idx]; }
	
	bool isInside(const float3&) const;
	bool isInside(const FBox&) const;
	void printInfo() const;

	const int2 dimensions() const { return m_size * node_size; }

protected:
	template <class NodeTest, class InstanceTest, class InstanceAction>
	void iterate(IRect grid_box, NodeTest, InstanceTest, InstanceAction) const;

	const IRect nodeCoords(const FBox &box) const;

	struct Node {
		Node();

		mutable FBox bbox;  // world space
		mutable IRect rect; // screen space
		mutable int obj_flags;

		//TODO: maybe all screen rects should be based on floats?
		int first_id;
		mutable bool is_dirty;
	} __attribute__((aligned(64)));

	struct Instance: public Object {
		int node_id;
		int next_id;
		int prev_id;
	} __attribute__((aligned(64)));

	//TODO: wrong for negative values
	static const int2 worldToGrid(const int2 &xz) { return xz / node_size; }
	static const int2 gridToWorld(const int2 &xz) { return xz * node_size; }

	int nodeAt(const int2 &grid_pos) const { return grid_pos.x + grid_pos.y * m_size.x; }
	void link(int cur_id, int next_id);
	void updateNode(int node_id) const;

protected:
	FBox m_bounding_box;
	int2 m_size;
	int2 m_max_bbox_size;
	vector<int2> m_row_rects;
	vector<int> m_free_list;
	vector<Node> m_nodes;
	vector<Instance> m_instances;
};

#endif

