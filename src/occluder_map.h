#ifndef OCCLUDER_MAP_H
#define OCCLUDER_MAP_h

#include "base.h"
#include "grid.h"

class OccluderMap {
public:
	enum { max_occluders = 32000 };

	OccluderMap(Grid*);
	OccluderMap(const OccluderMap&);
	void operator=(const OccluderMap&);

	int addOccluder(int representative_id);
	void removeOccluder(int occluder_id);
	void clear();

	void loadFromXML(const XMLDocument&);
	void saveToXML(const PodArray<int> &tile_ids, XMLDocument&) const;

	struct Occluder {
		Occluder() :parent_id(-1) { }

		vector<int> objects;
		int parent_id;
	};

	const Occluder &operator[](int id) const { return m_occluders[id]; }
	int size() const { return (int)m_occluders.size(); }
	
private:
	vector<Occluder> m_occluders;
	Grid *m_grid;
};

#endif
