/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "game/entity.h"
#include "game/tile_map.h"
#include "game/entity_map.h"
#include "game/level.h"
#include "game/path.h"
#include "navi_map.h"

namespace game {

	class Replicator {
	public:
		virtual ~Replicator() = default;
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

		// Use safer versions (with EntityRef & ObjectRef)
		Entity *refEntity(int index);

		int tileCount() const { return m_tile_map.size(); }
		int entityCount() const { return m_entity_map.size(); }
	
		template <class TEntity, class ...Args>
		EntityRef addNewEntity(const float3 &pos, const Args&... args) {
			PEntity entity(new TEntity(args...));
			entity->setPos(pos);
			return addEntity(std::move(entity));
		}

		// Takes ownership of entity
		EntityRef addEntity(PEntity&&, int index = -1);

		void removeEntity(EntityRef);

		void simulate(double time_diff);

		void updateNaviMap(bool full_recompute);
	
		double timeDelta() const { return m_time_delta; }
		double currentTime() const { return m_current_time; }
	
		bool findClosestPos(int3 &out, const int3 &source, const IBox &target_box, EntityRef agent) const;
		bool findPath(Path &out, const int3 &start, const int3 &end, EntityRef agent) const;

		TileMap &tileMap() { return m_level.tile_map; }
		const TileMap &tileMap() const { return m_level.tile_map; }
		
		const NaviMap *naviMap(int agent_size) const;

		const Grid::ObjectDef *refDesc(ObjectRef) const;
		const EntityMap::ObjectDef *refEntityDesc(int index) const;
		const TileMap::ObjectDef   *refTileDesc  (int index) const;

		const FBox refBBox(ObjectRef) const;
		const Tile *refTile(ObjectRef) const;
		Entity *refEntity(ObjectRef);
		Entity *refEntity(EntityRef);

		EntityRef toEntityRef(ObjectRef) const;
		EntityRef toEntityRef(int index) const;

		template <class TEntity, class TRef>
		TEntity *refEntity(TRef ref) {
			Entity *entity = refEntity(ref);
			if(entity && entity->typeId() == (EntityId::Type)TEntity::type_id)
				return static_cast<TEntity*>(entity);
			return nullptr;
		}

		ObjectRef findAny(const FBox &box, const FindFilter &filter = FindFilter()) const;
		void findAll(vector<ObjectRef> &out, const FBox &box, const FindFilter &filter = FindFilter()) const;
		Intersection trace(const Segment &segment, const FindFilter &filter = FindFilter()) const;

		bool isInside(const FBox&) const;

		Mode mode() const { return m_mode; }
		bool isClient() const { return m_mode == Mode::client; }
		bool isServer() const { return m_mode == Mode::server; }

		void replicate(int entity_id);
		void replicate(const Entity*);

		void playSound(SoundId, const float3 &pos);
		void playSound(const char*, const float3 &pos);

		bool sendOrder(POrder &&order, EntityRef actor_ref);
		
		int filterIgnoreIndex(const FindFilter &filter) const;

	private:
		const Mode m_mode;
		string m_map_name;

		double m_time_delta;
		double m_current_time;
		double m_last_time;

		double m_last_anim_frame_time;
		int m_anim_frame;

		//TODO: updating navi map on demand
		const NaviMap *accessNaviMap(const FBox &agent_box) const;

		//TODO: remove level
		Level		m_level;
		TileMap		&m_tile_map;
		EntityMap	&m_entity_map;

		vector<NaviMap> m_navi_maps;
		
		vector<pair<unique_ptr<Entity>, int>> m_replace_list;

		Replicator *m_replicator;
		friend class EntityWorldProxy;
	};

	typedef Ptr<World> PWorld;

}

#endif
