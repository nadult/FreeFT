#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "game/entity.h"
#include "game/projectile.h"
#include "navigation_map.h"
#include "bvh.h"
#include "grid.h"

namespace game {

	typedef Grid TileGrid;
	typedef Grid EntityGrid;
		
	class Intersection {
	public:
		Intersection(const Grid::ObjectDef *object = nullptr, bool is_entity = false, float distance = constant::inf)
			:m_object(object), m_is_entity(is_entity), m_distance(distance) { }

		const FBox boundingBox() const { return m_object? m_object->bbox : FBox::empty(); }
		Entity *entity() const { return isEntity()? (Entity*)m_object->ptr : nullptr; }

		bool isEntity() const { return m_is_entity && m_object; }
		bool isTile() const { return !m_is_entity && m_object; }
		bool isEmpty() const { return !m_object; }
		float distance() const { return m_distance; }

	private:
		const Grid::ObjectDef *m_object;
		float m_distance;
		bool m_is_entity;
	};

//	inline const Intersection &min(const Intersection &a, const Intersection &b) { return a.t < b.t? a : b; }

	class World {
	public:
		World();
		World(const char *file_name);
		~World();

		template <class T>
		T *addEntity(T *entity) {
			static_assert(std::is_base_of<Entity, T>::value, "T should be Entity-based");
			addEntity(PEntity(entity));
			return entity;
		}

		void addEntity(PEntity&&);
		void simulate(double time_diff);

		void updateNavigationMap(bool full_recompute);
	
		void spawnProjectile(PProjectile);
		void spawnProjectileImpact(PProjectileImpact);

		void addToRender(gfx::SceneRenderer&);

		double timeDelta() const { return m_time_delta; }
		double currentTime() const { return m_current_time; }
	
		// returns true if box collides with any of the tiles
		bool isColliding(const FBox &box, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
	
		vector<int2> findPath(int2 start, int2 end) const;

		const TileGrid &tileGrid() const { return m_tile_grid; }
		const NavigationMap &naviMap() const { return m_navi_map; }
		NavigationMap &naviMap() { return m_navi_map; }
	
		Intersection trace(const Segment &segment, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
		Intersection pixelIntersect(const int2 &screen_pos) const;

		bool isInside(const FBox&) const;

	private:
		template <class T>
		void handleContainer(vector<std::unique_ptr<T> > &objects, int frame_skip);
		
		double m_time_delta;
		double m_current_time;
		double m_last_time;
		double m_last_frame_time;

		TileGrid m_tile_grid;
		EntityGrid m_entity_grid;
		NavigationMap m_navi_map;

		vector<PEntity> m_entities;
		vector<PProjectile> m_projectiles;
		vector<PProjectileImpact> m_impacts;
	};

}

#endif
