#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "game/entity.h"
#include "navigation_map.h"
#include "tile_map.h"


namespace game {

	struct Intersection {
		Intersection() :entity(nullptr), t(1.0f / 0.0f) { }
		Intersection(Entity *entity, float t) :entity(entity), t(t) { }

		Entity *entity;
		float t;
	};

	class World {
	public:
		World();
		World(const char *file_name);

		template <class T>
		T *addEntity(T *entity) {
			static_assert(std::is_base_of<Entity, T>::value, "T should be Entity-based");
			addEntity(PEntity(entity));
			return entity;
		}

		void addEntity(PEntity&&);
		void simulate(double time_diff);
		void updateNavigationMap();

		void addToRender(gfx::SceneRenderer&);

		double timeDelta() const { return m_time_delta; }
		double currentTime() const { return m_current_time; }
	
		// returns true if box collides with any of the tiles
		bool isColliding(const IBox &box, const Entity *ignore = nullptr) const;
	
		vector<int2> findPath(int2 start, int2 end) const;

		const TileMap &tileMap() const { return m_tile_map; }
		const NavigationMap &naviMap() const { return m_navi_map; }
	
		Intersection intersectEntities(const Ray &ray, float tmin, float tmax) const;

	private:
		double m_time_delta;
		double m_current_time;
		double m_last_time;
		double m_last_frame_time;

		TileMap m_tile_map;
		NavigationMap m_navi_map;
		vector<PEntity> m_entities;
	};

}

#endif
