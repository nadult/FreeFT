/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "game/entity.h"
#include "game/tile_map.h"
#include "game/entity_map.h"
#include "game/level.h"
#include "navi_map.h"
#include <unordered_map>

namespace game {

	class Replicator {
	public:
		virtual ~Replicator() { }
		virtual void replicateEntity(int entity_id) { }
		virtual void replicateOrder(POrder&&, EntityRef) { }
	};

	class World: public RefCounter {
	public:
		enum class Mode {
			client,
			server,
			single_player,
		};

		World(const string &file_name, Mode mode = Mode::single_player, Replicator* = nullptr);
		~World();

		const char *mapName() const { return m_map_name.c_str(); }

		int entityCount() const { return m_entity_map.size(); }
	
		//TODO: access to entities only through EntityRef (which could also be initialized with
		//      simple index, then unique_id will not be checked)
		const Entity *getEntity(int id) const { return m_entity_map[id].ptr; }
		Entity *getEntity(int id) { return m_entity_map[id].ptr; }

		void removeEntity(Entity*);
		void removeEntity(int entity_id);

		// Takes ownership of entity
		int addEntity(PEntity&&, int index = -1);

		void simulate(double time_diff);

		void updateNaviMap(bool full_recompute);
	
		void addToRender(gfx::SceneRenderer&);

		double timeDelta() const { return m_time_delta; }
		double currentTime() const { return m_current_time; }
	
		vector<int3> findPath(const int3 &start, const int3 &end) const;

		const TileMap &tileMap() const { return m_level.tile_map; }
		const NaviMap &naviMap() const { return m_navi_map; }
		NaviMap &naviMap() { return m_navi_map; }
	
		const FBox refBBox(ObjectRef) const;
		const Tile *refTile(ObjectRef) const;
		Entity *refEntity(EntityRef);

		template <class TEntity>
		TEntity *refEntity(EntityRef ref) {
			Entity *entity = refEntity(ref);
			if(entity && entity->entityType() == (EntityId::Type)TEntity::type_id)
				return static_cast<TEntity*>(entity);
			return nullptr;
		}

		ObjectRef findAny(const FBox &box, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
		void findAll(vector<ObjectRef> &out, const FBox &box, const Entity *ignore = nullptr, ColliderFlags flags = collider_all) const;
		Intersection trace(const Segment &segment, const Entity *ignore = nullptr, int flags = collider_all) const;

		//TODO: option to ignore entities
		Intersection pixelIntersect(const int2 &screen_pos, int flags = collider_all) const;

		bool isInside(const FBox&) const;
		void updateVisibility(const FBox &main_bbox);

		Mode mode() const { return m_mode; }
		bool isClient() const { return m_mode == Mode::client; }
		bool isServer() const { return m_mode == Mode::server; }

		void replicate(int entity_id);
		void replicate(const Entity*);

		void playSound(SoundId, const float3 &pos);
		void playSound(const char*, const float3 &pos);

		bool sendOrder(POrder &&order, EntityRef actor_ref);

	private:
		const Mode m_mode;
		string m_map_name;

		double m_time_delta;
		double m_current_time;
		double m_last_time;

		double m_last_anim_frame_time;
		int m_anim_frame;

		//TODO: remove level
		Level		m_level;
		TileMap		&m_tile_map;
		EntityMap	&m_entity_map;
		NaviMap		m_navi_map;
		
		vector<pair<unique_ptr<Entity>, int>> m_replace_list;

		Replicator *m_replicator;
		friend class EntityWorldProxy;
	};

	typedef Ptr<World> PWorld;

}

#endif
