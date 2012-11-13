#ifndef UI_TILE_LIST_H
#define UI_TILE_LIST_H

#include "gfx/tile.h"

namespace ui
{

	struct TileListModel: public RefCounter {
		virtual ~TileListModel() { }
		virtual int size() const = 0;
		virtual const gfx::Tile* get(int idx, int &group_id) const = 0;
	};
	typedef Ptr<TileListModel> PTileListModel;

	struct AllTilesModel: public TileListModel {
		AllTilesModel();
		int size() const { return (int)m_tiles.size(); }
		const gfx::Tile* get(int idx, int&) const { return m_tiles[idx]; }

	protected:
		vector<const gfx::Tile*> m_tiles;
	};



	class TileList
	{
	public:
		struct Entry {
			const gfx::Tile *tile;
			int2 pos, size;
			int group_id, model_id, group_size;
			mutable bool is_selected;
			
			bool operator<(const Entry&) const;
		};

		TileList(int max_width, int spacing);
		void setModel(PTileListModel);

		const Entry* find(int2 pos) const;
		void update(); // call every time model changes

		int m_max_width, m_spacing;
		int m_height;

		int size() const { return (int)m_entries.size(); }
		const Entry& operator[](int idx) const { return m_entries[idx]; }
		Entry& operator[](int idx) { return m_entries[idx]; }

	private:
		vector<Entry> m_entries;
		PTileListModel m_model;
	};

}


#endif
