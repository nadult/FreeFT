#ifndef BVH_H
#define BVH_H


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

	static int maximumAxis(const FBox &box) {
		float3 size = box.size();
		if(size.x > size.y)
			return size.z > size.x? 2 : 0;
		else
			return size.z > size.y? 2 : 1;
	}

	static FBox split(const FBox &box, int axis, bool left) {
		FBox out_box = box;
		float mid = ((&box.min.x)[axis] + (&box.max.x)[axis]) * 0.5f;
		(&(left? out_box.max : out_box.min).x)[axis] = mid;
		return out_box;
	}

	BVH(const FBox &box) :m_box(box) {
		m_nodes.push_back(Node(m_box, -1, -1, maximumAxis(m_box)));
	}

	void addChild(int node_id, int child_id, const FBox &cur_box, int level) {
		DASSERT(child_id != -1);

		//printf("Add %d in %d lev: %d\n", child_id, node_id, level);
		if(m_nodes[node_id].isLeaf()) {
			Node &node = m_nodes[node_id];
		//	printf("Splitting leaf with: %d\n", node.child_id);
			int other_child = node.child_id;
			if(other_child == -1) {
				node.child_id = child_id;
				node.fit_box = m_objects[child_id].second;
				return;
			}
			node = Node(node.fit_box, -1, -1, maximumAxis(cur_box));
			node.is_empty = true;

			addChild(node_id, other_child, cur_box, level);
		}

		Node &node = m_nodes[node_id];
		node.is_empty = false;

	//	printf("Normal node with: %d %d\n", node.left, node.right);
		FBox left_box = split(cur_box, node.axis, true);
		FBox child_box = m_objects[child_id].second;
		bool go_left = left_box.isInside(child_box.min);
		node.fit_box = node.left == -1 && node.right == -1? child_box : node.fit_box + child_box;
	
		int next_id = go_left? node.left : node.right;
		if(next_id == -1) {
	//		printf("Adding %s leaf with: %d child\n", go_left? "left" : "right", child_id);
			(go_left? node.left : node.right) = (int)m_nodes.size();
			Node new_node(child_box, child_id);
			m_nodes.push_back(new_node);
			return;
		}

	//	printf("going %s\n", go_left? "left" : "right");
		FBox right_box = split(cur_box, node.axis, false);
		addChild(next_id, child_id, go_left? left_box : right_box, level + 1);
	}

	void removeChild(int parent_id, int node_id, int child_id, const FBox &box, int level) {
		Node &node = m_nodes[node_id];
		if(node.isLeaf()) {
			if(node.child_id == child_id) {
				node.is_empty = true;
				node.child_id = -1;
			}
			return;
		}

		FBox left_box = split(box, node.axis, true), right_box = split(box, node.axis, false);
		bool go_left = left_box.isInside(m_objects[child_id].second.min);
		int next_id = go_left? node.left : node.right;
		if(next_id == -1)
			return;
		removeChild(node_id, next_id, child_id, go_left? left_box : right_box, level + 1);

		node.is_empty = true;
		FBox fit_box = box;
		if(node.left != -1) {
			fit_box = m_nodes[node.left].fit_box;
			node.is_empty = m_nodes[node.left].is_empty;
		}
		if(node.right != -1) {
			const FBox &right_box = m_nodes[node.right].fit_box;
			fit_box = (node.left == -1? right_box : left_box + right_box);
			node.is_empty &= m_nodes[node.right].is_empty;
		}
		node.fit_box = fit_box;
	}

	void removeObject(int child_id) {
		removeChild(-1, 0, child_id, m_box, 0);
		m_objects[child_id].first = -1; //TODO
	}

	void addObject(T obj, FBox obj_box) {
		m_objects.push_back(make_pair(obj, obj_box));
		addChild(0, (int)m_objects.size() - 1, m_box, 0);
	}

	int print(int node_id, FBox box, int level) const {
	//	for(int n = 0; n < level; n++)
	//		printf("  ");
		
	//	printf("(%.2f %.2f %.2f | %.2f %.2f %.2f) ",
	//			box.min.x, box.min.y, box.min.z, box.width(), box.height(), box.depth());
		const Node &node = m_nodes[node_id];
		int count = 1;

		if(node.isLeaf())
			;//printf("Leaf: %d\n", node.child_id);
		else {
			//printf("Node: %d %d ax:%d is_empty:%d\n", node.left, node.right, (int)node.axis, (int)node.is_empty);
			FBox left_box, right_box;
			left_box = split(box, node.axis, true);
			right_box = split(box, node.axis, false);

			if(node.left != -1 && !node.is_empty)
				count += print(node.left, left_box, level + 1);
			if(node.right != -1 && !node.is_empty)
				count += print(node.right, right_box, level + 1);
		}

		return count;
	}

	int print() const {
		return print(0, m_box, 0);
	}

	bool isEmpty() const {
		return m_nodes.empty();
	}

	FBox m_box;
	vector<pair<T, FBox> > m_objects;
	vector<Node> m_nodes;
};



#endif
