#include "game/inventory.h"
#include "game/item.h"

namespace game {

	int Inventory::findItem(const ItemDesc *item) {
		for(int n = 0; n < size(); n++)
			if(m_entries[n].item == item)
				return n;
		return -1;
	}

	int Inventory::addItem(const ItemDesc *item, int count) {
		DASSERT(item && count >= 0);
		if(!count)
			return -1;

		int entry_id = findItem(item);
		if(entry_id != -1) {
			m_entries[entry_id].count += count;
			return entry_id;
		}

		m_entries.push_back(Entry{item, count});
		return size() - 1;
	}

	void Inventory::remove(int entry_id, int count) {
		DASSERT(entry_id >= 0 && entry_id < size());
		DASSERT(count >= 0);

		Entry &entry = m_entries[entry_id];
		entry.count -= count;
		if(entry.count <= 0) {
			m_entries[entry_id] = m_entries.back();
			m_entries.pop_back();
		}
	}

	float Inventory::weight() const {
		double sum = 0.0;
		for(int n = 0; n < size(); n++)
			sum += double(m_entries[n].item->weight) * double(m_entries[n].count);
		return float(sum);
	}

}
