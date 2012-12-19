#ifndef BVH_H
#define BVH_H

#include "base.h"

// Requirements:
// - fast element modification, it would be nice if tree quality wouldn't degrade
// - quick full rebuild in NlgN
// - raytracing, intersction tests with bounding boxes (collision tests)
// - intersection with rects in screen space
// - support for entities & tiles
//
// Two types:
// BVH<gfx::Tile*>
// BVH<game::Entity*> // TODO: use simple pointers or maybe EntityRef?
//
// Space subdivision:
// - optimial (or close to)? probably would degrade quickly, with lots ot modifications
// - split in the middle, until there is only single object in the node (BIH style)
//   - for stringy parts of the tree we can add additional jump pointer which would be ignored
//      when tree is updated (after the update is finished it will be updated too)
//TODO: better removal of objects
template <class T>
class BVH {
public:
	class Node {
	public:
		Node() { }
		Node(FBox fit_box, int child_id)
			:child_id(child_id), is_leaf(true), fit_box(fit_box), is_empty(child_id != -1) { }
		Node(FBox fit_box, int left, int right, int axis)
			:left(left), right(right), axis(axis), fit_box(fit_box), is_leaf(false), is_empty(true) { }

		// there are no empty leafs
		bool isLeaf() const { return is_leaf; }

		FBox fit_box;
		union {
			struct { int left, right; };
			struct { int child_id; };
		};
		char axis;
		char is_leaf;
		char is_empty;
	};

	struct Object {
		Object(T object, int node_id) :object(object), node_id(node_id), next_child_id(-1) { }
		Object() :node_id(-1), next_child_id(-1) { }

		T object;
		int node_id, next_child_id;
	};

	struct Intersection {
		Intersection() :distance(constant::inf), object_id(-1) { }
		Intersection(int object_id, float distance) :object_id(object_id), distance(distance) { }

		int object_id;
		float distance;
	};

	BVH() :m_box(0, 0, 0, 0, 0, 0) { }
	BVH(const FBox &box) :m_box(box) {
		m_nodes.push_back(Node(m_box, -1, -1, maximumAxis(m_box)));
	}

	// call this method if objects bounding box has changed
	void updateObject(int idx);
	void removeObject(int idx);
	void addObject(T obj);


	Intersection intersect(const Segment &segment, int node_id = 0) const;

	int print(int node_id, FBox box, int level) const;

	int print() const {
		return print(0, m_box, 0);
	}

	FBox check(int node_id);

	void printStats() const;

	bool isEmpty() const { return m_nodes.empty(); }
	int size() const { return (int)m_objects.size(); }

	const T &operator[](int idx) const { return m_objects[idx].object; }
	T &operator[](int idx) { return m_objects[idx].object; }

protected:
	static int maximumAxis(const FBox &box);
	static FBox split(const FBox &box, int axis, bool left);
	static bool canSplitNode(const FBox &box);

	int  addChild(int node_id, int child_id, const FBox &cur_box, const FBox &child_box);
	void removeChild(int node_id, int child_id, const FBox &box, const FBox &child_box);

	FBox m_box;
	vector<Object> m_objects;
	vector<Node> m_nodes;
};



#endif
