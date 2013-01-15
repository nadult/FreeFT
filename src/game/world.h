#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "game/entity.h"
#include "game/projectile.h"
#include "game/tile_map.h"
#include "game/entity_map.h"
#include "navi_map.h"

namespace game {

	class Intersection {
	public:
		Intersection(const   TileMap::ObjectDef *object = nullptr, float distance = constant::inf)
			:m_tile(object), m_is_entity(false), m_distance(distance) { }
		Intersection(const EntityMap::ObjectDef *object, float distance = constant::inf)
			:m_entity(object), m_is_entity(true), m_distance(distance) { }

		const FBox boundingBox() const { return m_entity? m_entity->bbox : FBox::empty(); }

		const Tile *tile() const { return   isTile()?   m_tile->ptr : nullptr; }
		  Entity *entity() const { return isEntity()? m_entity->ptr : nullptr; }

		bool isEntity() const { return  m_is_entity && m_entity; }
		bool   isTile() const { return !m_is_entity && m_tile; }

		bool isEmpty() const { return !m_entity; }
		float distance() const { return m_distance; }

	private:
		union {
			const EntityMap::ObjectDef *m_entity;
			const   TileMap::ObjectDef *m_tile;
		};
		float m_distance;
		bool m_is_entity;
	};

//	inline const Intersection &min(const Intersection &a, const Intersection &b) { return a.t < b.t? a : b; }

	//TODO: make sure to remove entities from grid also
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

		void updateNaviMap(bool full_recompute);
	
		void spawnProjectile(PProjectile);
		void spawnProjectileImpact(PProjectileImpact);

		void addToRender(gfx::SceneRenderer&);

		double timeDelta() const { return m_time_delta; }
		double currentTime() const { return m_current_time; }
	
		// returns true if box collides with any of the tiles
		bool isColliding(const FBox &box, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
	
		vector<int3> findPath(const int3 &start, const int3 &end) const;

		const TileMap &tileMap() const { return m_tile_map; }
		const NaviMap &naviMap() const { return m_navi_map; }
		NaviMap &naviMap() { return m_navi_map; }
	
		Intersection trace(const Segment &segment, const Entity *ignore = nullptr, int flags = collider_all) const;
		Intersection pixelIntersect(const int2 &screen_pos, int flags = collider_all) const;

		bool isInside(const FBox&) const;
		void updateVisibility(const FBox &main_bbox);

	private:
		template <class T>
		void handleContainer(vector<std::unique_ptr<T> > &objects, int frame_skip);
		
		double m_time_delta;
		double m_current_time;
		double m_last_time;
		double m_last_frame_time;

		TileMap   m_tile_map;
		EntityMap m_entity_map;
		NaviMap m_navi_map;

		vector<PEntity> m_entities;
		vector<PProjectile> m_projectiles;
		vector<PProjectileImpact> m_impacts;
	};

}

#endif
