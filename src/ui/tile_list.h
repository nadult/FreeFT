#ifndef UI_TILE_LIST_H
#define UI_TILE_LIST_H

#include "game/tile.h"

class TileGroup;

namespace ui
{

	namespace TileFilter {
		enum Type {
			all,
			floors,
			walls,
			roofs,
			objects,
			other,

			count,
		};

		const char **strings();
		bool test(const game::Tile *tile, int filter);
	};

	struct TileListModel: public RefCounter {
		virtual ~TileListModel() { }
		virtual int size() const = 0;
		virtual const game::Tile* get(int idx, int &group_id) const = 0;
	};
	typedef Ptr<TileListModel> PTileListModel;

	PTileListModel allTilesModel();
	PTileListModel groupedTilesModel(const TileGroup&, bool only_uniform);
	PTileListModel filteredTilesModel(PTileListModel, TileFilter::Type);


	class TileList
	{
	public:
		struct Entry {
			const game::Tile *tile;
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
