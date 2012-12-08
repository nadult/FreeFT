#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "game/entity.h"
#include "game/projectile.h"
#include "navigation_map.h"
#include "tile_map.h"

namespace game {

	class WorldElement {
	public:
		WorldElement();
		WorldElement(Entity *entity);
		WorldElement(const TileMap *tile_map, int tile_node_id, int tile_instance_id);

		const FBox boundingBox() const;
		Entity *entity() const { return m_tile_node_id == -1? m_entity : nullptr; }

		bool isEntity() const { return m_tile_node_id == -1 && m_entity != nullptr; }
		bool isTile() const { return m_tile_node_id != -1; }
		bool isEmpty() const { return m_tile_node_id == -1 && m_entity == nullptr; }

	private:
		union {
			Entity *m_entity;
			const TileMap *m_tile_map;
		};
		int m_tile_node_id;
		int m_tile_instance_id;
	};

	struct Intersection : public WorldElement {
		Intersection() :distance(constant::inf) { }
		Intersection(const WorldElement &element, float distance) :WorldElement(element), distance(distance) { }

		float distance;
	};

//	inline const Intersection &min(const Intersection &a, const Intersection &b) { return a.t < b.t? a : b; }

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

		void updateNavigationMap(bool full_recompute);
	
		void spawnProjectile(int type, const float3 &pos, const float3 &target, Entity *spawner);
		void spawnProjectileImpact(int type, const float3 &pos);

		void addToRender(gfx::SceneRenderer&);

		double timeDelta() const { return m_time_delta; }
		double currentTime() const { return m_current_time; }
	
		// returns true if box collides with any of the tiles
		bool isColliding(const FBox &box, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
	
		vector<int2> findPath(int2 start, int2 end) const;

		const TileMap &tileMap() const { return m_tile_map; }
		const NavigationMap &naviMap() const { return m_navi_map; }
		NavigationMap &naviMap() { return m_navi_map; }
	
		Intersection intersect(const Segment &segment, const Entity *ignore = nullptr,
								ColliderFlags flags = collider_all) const;

		bool isInside(const FBox&) const;

	private:
		template <class T>
		void handleContainer(vector<std::unique_ptr<T> > &objects, int frame_skip);
		
		double m_time_delta;
		double m_current_time;
		double m_last_time;
		double m_last_frame_time;

		TileMap m_tile_map;
		NavigationMap m_navi_map;

		vector<PEntity> m_entities;
		vector<PProjectile> m_projectiles;
		vector<PProjectileImpact> m_impacts;
	};

}

#endif
