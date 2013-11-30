/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GRID_H
#define GRID_H

#include "base.h"

// When accessing objects directly, you have to check ptr for null,
// because some of the objects may be invalid; you don't have to check
// if you're accesing object at index returned from one of the interection
// functions.
//
// TODO: single Object - Grid tests are too costly, make functions
// for testing multiple objects at once
class Grid {
public:
	enum {
		node_size = 24,
		max_height = 256,

		collider_flags		= 0x00ffffff, // at least one type flag have to be set
		functional_flags	= 0xff000000, // all functional flags have to be set

		//TODO: currently flag test passes if at least one bit is set,
		// it would be useful to check if all bits from within some range are set
	};

	template <typename PtrBase>
	struct TObjectDef {
		TObjectDef(PtrBase* ptr = nullptr, const FBox &bbox = FBox::empty(),
				const IRect &rect = IRect::empty(), int flags = collider_flags)
			:ptr(ptr), bbox(bbox), rect_pos(rect.min), rect_size(rect.size()), flags(flags),
				occluder_id(-1), occluded_by(-1) {
			DASSERT(rect.size() == (int2)rect_size);
		}

		const IRect rect() const { return IRect(rect_pos, rect_pos + rect_size); }

		PtrBase *ptr;
		FBox bbox;
		int2 rect_pos;
		short2 rect_size;

		int flags;

		short occluder_id;
		short occluded_by;
	};

	static bool flagTest(int obj_flags, int test_flags);

	typedef TObjectDef<void> ObjectDef;

	Grid(const int2 &dimensions = int2(0, 0));

	int add(const ObjectDef&);
	void remove(int idx);
	void update(int idx, const ObjectDef&);
	void updateNodes();

	int findAny(const FBox &box, int ignored_id = -1, int flags = collider_flags) const;
	void findAll(vector<int> &out, const FBox &box, int ignored_id = -1, int flags = collider_flags) const;

	pair<int, float> trace(const Segment &segment, int ignored_id = -1, int flags = collider_flags) const;
	
	void findAll(vector<int> &out, const IRect &view_rect, int flags = collider_flags) const;
	int pixelIntersect(const int2 &pos, bool (*pixelTest)(const ObjectDef&, const int2 &pos), int flags = collider_flags) const;

	int size() const { return (int)m_objects.size(); }
	const ObjectDef &operator[](int idx) const { return m_objects[idx]; }
	ObjectDef &operator[](int idx) { return m_objects[idx]; }
	
	bool isInside(const float3&) const;
	bool isInside(const FBox&) const;
	void printInfo() const;

	const int2 dimensions() const { return m_size * node_size; }

	void swap(Grid&);
	void clear();

protected:
	const IRect nodeCoords(const FBox &box) const __attribute__((noinline));

	struct Node {
		Node();

		//TODO: maybe all screen rects should be based on floats?
		mutable FBox bbox;  // world space
		mutable IRect rect; // screen space
		mutable int obj_flags;

		int first_id, first_overlap_id;
		int size;
		mutable bool is_dirty;
	} __attribute__((aligned(64)));

	struct Overlap {
		int object_id;
		int next_id;
	};

	struct Object: public ObjectDef {
		int node_id; // -1 means that its overlapping more than one node
		int next_id;
		int prev_id :31;
		mutable int is_disabled :1;
	} __attribute__((aligned(64)));

	//TODO: wrong for negative values
	static const int2 worldToGrid(const int2 &xz) { return xz / node_size; }
	static const int2 gridToWorld(const int2 &xz) { return xz * node_size; }
	bool isInsideGrid(const int2 &p) const { return p.x >= 0 && p.y >= 0 && p.x < m_size.x && p.y < m_size.y; }

	int nodeAt(const int2 &grid_pos) const { return grid_pos.x + grid_pos.y * m_size.x; }
	void link(int cur_id, int next_id);
	void updateNode(int node_id) const __attribute((noinline));
	void updateNode(int node_id, const ObjectDef&) const __attribute((noinline));
	int extractObjects(int node_id, const Object **out, int ignored_id = -1, int flags = 0) const;

protected:
	void disableOverlap(const Object*) const;
	void clearDisables() const;

	FBox m_bounding_box;
	int2 m_size;
	vector<int2> m_row_rects;
	vector<int> m_free_list;
	vector<int> m_free_overlaps; // TODO: do as in TextureCache
	vector<Overlap> m_overlaps;
	vector<Node> m_nodes;
	vector<Object> m_objects;

	mutable vector<int> m_disabled_overlaps;
};

#endif

