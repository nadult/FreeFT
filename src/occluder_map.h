// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "base.h"
#include "grid.h"

class OccluderMap {
public:
	static constexpr int max_occluders = 32000;

	OccluderMap(Grid&);
	OccluderMap(OccluderMap&&);
	OccluderMap &operator=(OccluderMap&&);

	int addOccluder(int representative_id, int min_height);
	void removeOccluder(int occluder_id);
	void clear();

	void loadFromXML(const XmlDocument&);
	void saveToXML(const PodVector<int> &tile_ids, XmlDocument&) const;

	struct Occluder {
		FBox bbox;
		vector<int> objects;
	};

	const Occluder &operator[](int id) const { return m_occluders[id]; }
	Occluder &operator[](int id) { return m_occluders[id]; }
	int size() const { return (int)m_occluders.size(); }

	bool isUnder(int lower_id, int upper_id) const;

	vector<FBox> computeBBoxes(int occluder_id, bool minimize) const;
	bool verifyBBoxes(int occluder_id, const vector<FBox>&) const;

private:
	vector<Occluder> m_occluders;
	Grid &m_grid;

	friend class OccluderConfig;
};

class OccluderConfig {
public:
	OccluderConfig(const OccluderMap &map);

	bool update();
	bool update(const FBox &spectator);

	template <class Container>
	void filterVisible(const Container &elements, vector<int> &indices) const {
		int count = (int)indices.size();

		for(int i = 0; i < count; i++)
			if(!isVisible(elements[indices[i]].occluder_id))
				indices[i--] = indices[--count];
		indices.resize(count);
	}

	template <class Container>
	void setVisibilityFlag(Container &elements, int flag) const {
		for(int n = 0; n < (int)elements.size(); n++) {
			auto &element = elements[n];
			bool is_visible = isVisible(element.occluder_id);
			element.flags = (element.flags & ~flag) | (is_visible? flag : 0);
		}
	}

	struct OccluderState {
		OccluderState() :is_visible(true), is_overlapping(true) { }

		bool is_visible : 1;
		bool is_overlapping : 1;
	};

	void setVisible(int occluder_id, bool is_visible);
	bool isVisible(int occluder_id) const;

private:
	const OccluderMap &m_map;
	vector<OccluderState> m_states;

};
