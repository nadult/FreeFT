#include "bvh.h"


template <class T>
int BVH<T>::maximumAxis(const FBox &box) {
	float3 size = box.size();
	if(size.x > size.y)
		return size.z > size.x? 2 : 0;
	else
		return size.z > size.y? 2 : 1;
}

template <class T>
FBox BVH<T>::split(const FBox &box, int axis, bool left) {
	FBox out_box = box;
	float mid = ((&box.min.x)[axis] + (&box.max.x)[axis]) * 0.5f;
	(&(left? out_box.max : out_box.min).x)[axis] = mid;
	return out_box;
}

template <class T>
bool BVH<T>::canSplitNode(const FBox &box) {
	return box.width() >= 16.0f || box.height() >= 16.0f || box.depth() >= 16.0f;
}

template <class T>
int BVH<T>::addChild(int node_id, int child_id, const FBox &cur_box, const FBox &child_box) {
	DASSERT(child_id != -1);

	//printf("Add %d in %d lev: %d\n", child_id, node_id, level);
	if(m_nodes[node_id].isLeaf()) {
		Node &node = m_nodes[node_id];
	//	printf("Splitting leaf with: %d\n", node.child_id);
	
		int other_child = node.child_id;
		if(other_child == -1 || !canSplitNode(cur_box)) {
			node.is_empty = false;
			node.child_id = child_id;
			node.fit_box = (other_child == -1? child_box : sum(node.fit_box, child_box));
			m_objects[child_id].next_child_id = other_child;
			return node_id;
		}

		node = Node(node.fit_box, -1, -1, maximumAxis(cur_box));
		node.is_empty = true;
		DASSERT(m_objects[other_child].next_child_id == -1);
		addChild(node_id, other_child, cur_box, (FBox)m_objects[other_child].object.boundingBox());
	}

	Node &node = m_nodes[node_id];
	node.is_empty = false;

//	printf("Normal node with: %d %d\n", node.left, node.right);
	FBox left_box = split(cur_box, node.axis, true);
	bool go_left = left_box.isInside(child_box.min);
	node.fit_box = node.left == -1 && node.right == -1? child_box : sum(node.fit_box, child_box);

	int next_id = go_left? node.left : node.right;
	if(next_id == -1) {
//		printf("Adding %s leaf with: %d child\n", go_left? "left" : "right", child_id);
		(go_left? node.left : node.right) = (int)m_nodes.size();
		Node new_node(child_box, child_id);
		m_nodes.push_back(new_node);
		return (int)m_nodes.size() - 1;
	}

//	printf("going %s\n", go_left? "left" : "right");
	FBox right_box = split(cur_box, node.axis, false);
	return addChild(next_id, child_id, go_left? left_box : right_box, child_box);
}

template <class T>
FBox BVH<T>::check(int node_id) {
	Node &node = m_nodes[node_id];
	FBox box(0, 0, 0, 0, 0, 0);
	int count = 0;

	if(node.is_empty)
		return box;

	if(node.isLeaf()) {
		int child_id = node.child_id;
		if(child_id != -1) {
			box = (FBox)m_objects[child_id].object.boundingBox();
			child_id = m_objects[child_id].next_child_id;
			count++;
		}
		while(child_id != -1) {
			count++;
			box = sum(box, (FBox)m_objects[child_id].object.boundingBox());
			child_id = m_objects[child_id].next_child_id;
		}	
	}
	else {
		if(node.left != -1)
			box = check(node.left);
		if(node.right != -1) {
			FBox right = check(node.right);
			box = node.left == -1 || m_nodes[node.left].is_empty? right : sum(box, right);
		}
	}
	
	if(!(node.fit_box == box)) {
		printf("node %d\n", count);
		printf("%f %f %f %f %f %f\n",
				node.fit_box.min.x,
				node.fit_box.min.y,
				node.fit_box.min.z,
				node.fit_box.max.x,
				node.fit_box.max.y,
				node.fit_box.max.z);
		printf("%f %f %f %f %f %f\n",
				box.min.x,
				box.min.y,
				box.min.z,
				box.max.x,
				box.max.y,
				box.max.z);
		DASSERT(0);
	}

	return box;
}

template <class T>
void BVH<T>::removeChild(int node_id, int child_id, const FBox &box, const FBox &child_box) {
	Node &node = m_nodes[node_id];
	if(node.isLeaf()) {
		int cur_id = node.child_id, prev_id = -1;
		while(cur_id != -1 && cur_id != child_id) {
			prev_id = cur_id;
			cur_id = m_objects[child_id].next_child_id;
		}

		if(cur_id == child_id) {
			int next_child_id = m_objects[child_id].next_child_id;
			(prev_id == -1? node.child_id : m_objects[prev_id].next_child_id) = next_child_id;
			if( !(node.is_empty = node.child_id == -1) ) {
				FBox fit_box = m_objects[node.child_id].object.boundingBox();
				cur_id = m_objects[node.child_id].next_child_id;
				while(cur_id != -1) {
					fit_box = sum(fit_box, m_objects[cur_id].object.boundingBox());
					cur_id = m_objects[cur_id].next_child_id;
				}
				node.fit_box = fit_box;
			}
		}
		return;
	}

	FBox left_box = split(box, node.axis, true), right_box = split(box, node.axis, false);
	bool go_left = left_box.isInside(child_box.min);
	int next_id = go_left? node.left : node.right;
	if(next_id == -1)
		return;
	removeChild(next_id, child_id, go_left? left_box : right_box, child_box);

	node.is_empty = true;
	FBox fit_box = box;
	if(node.left != -1) {
		fit_box = m_nodes[node.left].fit_box;
		node.is_empty = m_nodes[node.left].is_empty;
	}
	if(node.right != -1) {
		const FBox &right_box = m_nodes[node.right].fit_box;
		fit_box = (node.left == -1 || m_nodes[node.left].is_empty? right_box : sum(left_box, right_box));
		node.is_empty &= m_nodes[node.right].is_empty;
	}
	node.fit_box = fit_box;
}

template <class T>
typename BVH<T>::Intersection BVH<T>::intersect(const Segment &segment, int node_id) const {
	const Node *node = &m_nodes[node_id];
	float dist = intersection(segment, node->fit_box);

	if(dist == constant::inf)
		return Intersection();

	while(!node->isLeaf() && (node->left == -1 || node->right == -1))
		node = &m_nodes[node->left == -1? node->right : node->left];
	
	if(node->isLeaf()) {
		Intersection isect;
		int cur_id = node->child_id;
		while(cur_id != -1) {
			float dist = intersection(segment, (FBox)m_objects[cur_id].object.boundingBox());
			if(dist < isect.distance)
				isect = Intersection(cur_id, dist);
			cur_id = m_objects[cur_id].next_child_id;
		}

		return isect;
	}

	bool left_first = (&segment.dir().x)[(int)node->axis] > 0.0f;
	int first = node->left, second = node->right;
	if(left_first)
		swap(first, second);

	Intersection isect1 = intersect(segment, first);
	Intersection isect2 = intersect(Segment(Ray(segment), segment.min, min(segment.max, isect1.distance)), second);

	return isect1.distance < isect2.distance? isect1 : isect2;
}

template <class T>
int BVH<T>::print(int node_id, FBox box, int level) const {
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

template <class T>
void BVH<T>::printStats() const {
	printf("BVH info:\nNodes: %d (%.2f KB)\nObjects: %d (%.2f KB)\n",
			(int)m_nodes.size(), (float)m_nodes.size() * sizeof(Node) / 1024.0f,
			(int)m_objects.size(), (float)m_objects.size() * sizeof(Object) / 1024.0f);
	//TODO
}

template <class T>
void BVH<T>::removeObject(int child_id) {
	removeChild(-1, 0, child_id, m_box, m_objects[child_id].boundingBox());
	m_objects[child_id] = m_objects.back();
	m_nodes[m_objects[child_id].node_id].child_id = child_id;
	m_objects.pop_back();  // TODO: make sure that there is no unnecassary allocations / deallocations
}

template <class T>
void BVH<T>::addObject(T obj) {
	m_objects.push_back(Object(obj, -1));
	m_objects.back().node_id = addChild(0, (int)m_objects.size() - 1, m_box, (FBox)obj.boundingBox());
}

template <class T>
void BVH<T>::updateObject(int idx) {
	const FBox bbox(m_objects[idx].object.boundingBox());
	removeChild(-1, 0, idx, m_box, bbox);
	m_objects[idx].node_id = addChild(0, idx, m_box, bbox);
}

