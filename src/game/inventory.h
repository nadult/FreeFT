#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include "base.h"

namespace game {

	class ItemDesc;

	class Inventory {
	public:
		struct Entry {
			const ItemDesc *item;
			int count;
		};

		int addItem(const ItemDesc *item, int count);
		int findItem(const ItemDesc *item);
		void remove(int entry_id, int count);
		float weight() const;

		int size() const { return (int)m_entries.size(); }
		const Entry &operator[](int idx) const { return m_entries[idx]; }

	protected:
		vector<Entry> m_entries;
	};



}


#endif
