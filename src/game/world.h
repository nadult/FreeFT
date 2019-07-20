// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "game/entity.h"
#include "game/tile_map.h"
#include "game/entity_map.h"
#include "game/level.h"
#include "game/path.h"
#include "game/trigger.h"
#include "navi_map.h"
#include <thread>

namespace game {

	class Replicator {
	public:
		virtual ~Replicator() = default;
		virtual void replicateEntity(int entity_id) { }
		virtual void replicateOrder(POrder&&, EntityRef) { }
		virtual void sendMessage(net::TempPacket&, int target_id) = 0;
		//TODO: inform replicator about entity priorities
		// most basic: which entity belongs to which client
	};

	class GameMode;

	class World {
	public:
		enum class Mode { //TODO: better name
			client,
			server,
			single_player,
		};

		World(string map_name, Mode mode = Mode::single_player);
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
		float random();
	
		bool findClosestPos(int3 &out, const int3 &source, const IBox &target_box, EntityRef agent) const;
		bool findPath(Path &out, const int3 &start, const int3 &end, EntityRef agent) const;

		TileMap &tileMap() { return m_level.tile_map; }
		const TileMap &tileMap() const { return m_level.tile_map; }
		
		//TODO: updating navi map on demand
		const NaviMap *naviMap(int agent_size) const;
		const NaviMap *naviMap(EntityRef agent) const;
		const NaviMap *naviMap(const FBox &agent_box) const;

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
			return dynamic_cast<TEntity*>(refEntity(ref));
		}

		ObjectRef findAny(const FBox &box, const FindFilter &filter = FindFilter()) const;
		void findAll(vector<ObjectRef> &out, const FBox &box, const FindFilter &filter = FindFilter()) const;
		Intersection trace(const Segment3F &, const FindFilter &filter = FindFilter()) const;
		void traceCoherent(const vector<Segment3F> &, vector<Intersection> &out, const FindFilter &filter = FindFilter()) const;

		bool isInside(const FBox&) const;
		bool isVisible(const float3 &eye_pos, EntityRef target, EntityRef ignore, int density) const;

		Mode mode() const { return m_mode; }
		bool isClient() const { return m_mode == Mode::client; }
		bool isServer() const { return m_mode == Mode::server; }

		template <class TGameMode, class ...Args>
		void assignGameMode(const Args&... args) {
			ASSERT(!m_game_mode);
			m_game_mode.reset(new TGameMode(*this, args...));
		}
		GameMode *gameMode() const { return m_game_mode.get(); }
		Maybe<GameModeId> gameModeId() const;

		void setReplicator(Replicator*);
		void replicate(int entity_id);
		void replicate(const Entity*);
		void onMessage(Stream&, int source_id);

		//TODO: single function with replication as a parameter
		// in some objects we can check if sound is played in the first frame or not
		void playSound(SoundId, const float3 &pos, SoundType sound_type = SoundType::normal);
		void replicateSound(SoundId, const float3 &pos, SoundType sound_type = SoundType::normal);

		bool sendOrder(POrder &&order, EntityRef actor_ref);
		void sendMessage(net::TempPacket&, int target_id = -1);
		
		int filterIgnoreIndex(const FindFilter &filter) const;

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

		vector<NaviMap> m_navi_maps;
		
		vector<pair<unique_ptr<Entity>, int>> m_replace_list;

		PGameMode m_game_mode;

		Replicator *m_replicator;
		friend class EntityWorldProxy;
	};

	using PWorld = shared_ptr<World>;

}

#endif
