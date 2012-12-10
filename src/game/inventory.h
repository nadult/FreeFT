#ifndef GAME_INVENTORY_H
#define GAME_INVENTORY_H

#include "base.h"
#include "game/item.h"

namespace game {

	class Inventory {
	public:
		struct Entry {
			Item item;
			int count;
		};

		int add(const Item &item, int count);
		int find(const Item &item);
		void remove(int entry_id, int count);
		float weight() const;

		int size() const { return (int)m_entries.size(); }
		const Entry &operator[](int idx) const { return m_entries[idx]; }

	protected:
		vector<Entry> m_entries;
	};



}


#endif
