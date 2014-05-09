/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "io/single_player_loop.h"
#include "game/actor.h"
#include "game/world.h"
#include "game/trigger.h"
#include "sys/config.h"

using namespace game;

namespace io {
	
	game::PWorld createWorld(const string &map_name) {
		game::PWorld world(new World(map_name, World::Mode::single_player));
		return world;
	}

	SinglePlayerLoop::SinglePlayerLoop(game::PWorld world) {
		DASSERT(world && world->mode() == World::Mode::single_player);
		m_world = world;

		Config config = loadConfig("game");
		m_time_multiplier = config.time_multiplier;

		Trigger *spawn_zone = nullptr;

		for(int n = 0; n < m_world->entityCount(); n++) {
			Actor *actor = m_world->refEntity<Actor>(n);
			if(actor && actor->factionId() != 0)
				actor->attachAI<SimpleAI>();

			Trigger *trigger = m_world->refEntity<Trigger>(n);
			if(trigger && trigger->classId() == TriggerClassId::spawn_zone)
				spawn_zone = trigger;
		}

		if(!spawn_zone)
			THROW("Spawn zone not found!\n");

		EntityRef actor_ref; {
			PEntity actor = (PEntity)new Actor(getProto("male", ProtoId::actor));

			FBox spawn_box = spawn_zone->boundingBox();
			float3 spawn_pos = spawn_box.center();

			actor->setPos(spawn_pos);
			while(!m_world->findAny(actor->boundingBox(), {Flags::all | Flags::colliding})) {
				spawn_pos.y -= 1.0f;
				actor->setPos(spawn_pos);
			}

			spawn_pos.y += 1.0f;
			actor->setPos(spawn_pos);
			actor_ref = m_world->addEntity(std::move(actor));
		}

		if( Actor *actor = m_world->refEntity<Actor>(actor_ref) ) {
			auto &inventory = actor->inventory();
			inventory.add(findProto("plasma_rifle", ProtoId::item_weapon), 1);
			inventory.add(findProto("laser_rifle", ProtoId::item_weapon), 1);
			inventory.add(findProto("heavy_laser_rifle", ProtoId::item_weapon), 1);
			inventory.add(findProto("uzi", ProtoId::item_weapon), 1);
			inventory.add(findProto("rocket_launcher", ProtoId::item_weapon), 1);
			inventory.add(findProto("flamer", ProtoId::item_weapon), 1);
			inventory.add(findProto("ak47", ProtoId::item_weapon), 1);
			inventory.add(findProto("power_armour", ProtoId::item_armour), 1);
		}
		
		m_controller.reset(new Controller(gfx::getWindowSize(), m_world, actor_ref, config.profiler_enabled));
	}

	bool SinglePlayerLoop::tick(double time_diff) {
		m_controller->update();

		time_diff *= m_time_multiplier;
		m_world->simulate(time_diff);
		m_controller->updateView(time_diff);

		m_controller->draw();
		return !gfx::isKeyPressed(gfx::Key_esc);
	}

}
