/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef OCCLUDER_MAP_H
#define OCCLUDER_MAP_H

#include "base.h"
#include "grid.h"

class OccluderMap {
public:
	enum { max_occluders = 32000 };

	OccluderMap(Grid&);

	int addOccluder(int representative_id);
	void removeOccluder(int occluder_id);
	void clear();

	void loadFromXML(const XMLDocument&);
	void saveToXML(const PodArray<int> &tile_ids, XMLDocument&) const;

	struct Occluder {
		Occluder() :is_visible(true), is_overlapping(false) { }

		FBox bbox;
		vector<int> objects;
		bool is_visible;
		bool is_overlapping;
	};

	const Occluder &operator[](int id) const { return m_occluders[id]; }
	Occluder &operator[](int id) { return m_occluders[id]; }
	int size() const { return (int)m_occluders.size(); }

	// returns true if anything has changed
	bool updateVisibility(const FBox &main_bbox);

	bool isUnder(int lower_id, int upper_id) const;

	vector<FBox> computeBBoxes(int occluder_id, bool minimize) const;
	bool verifyBBoxes(int occluder_id, const vector<FBox>&) const;
	
private:
	vector<Occluder> m_occluders;
	Grid &m_grid;
};

#endif
