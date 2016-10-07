/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/single_player_mode.h"
#include "game/world.h"
#include "game/actor.h"

namespace game {

	SinglePlayerMode::SinglePlayerMode(World &world, Character character)
		:GameMode(world, 0) {
		attachAIs();
//		for(int n =0; n < m_world.entityCount(); n++)
//			if(m_world.refEntity<Trigger>(n) && m_world.refEntity<Trigger>(n)->factionId() > 0)
//				m_world.removeEntity(m_world.refEntity(n)->ref());

		GameClient client;
		client.nick_name = character.name();
		client.pcs.emplace_back(PlayableCharacter(character, CharacterClass::defaultId()));
		m_clients[0] = client;

		m_pc = pc(PCIndex(0, 0));
		DASSERT(m_pc);

		ASSERT(!world.isServer() && !world.isClient());

		EntityRef spawn_zone = findSpawnZone(0);
		if(!spawn_zone)
			THROW("Spawn zone not found!\n");

		ActorInventory inventory; {
			inventory.add(findProto("plasma_rifle", ProtoId::weapon), 1);
			inventory.add(findProto("laser_rifle", ProtoId::weapon), 1);
			inventory.add(findProto("heavy_laser_rifle", ProtoId::weapon), 1);
			inventory.add(findProto("uzi", ProtoId::weapon), 1);
			inventory.add(findProto("rocket_launcher", ProtoId::weapon), 1);
			inventory.add(findProto("ak47", ProtoId::weapon), 1);
			inventory.add(findProto("beretta", ProtoId::weapon), 1);
			inventory.add(findProto("gatling_laser", ProtoId::weapon), 1);
			inventory.add(findProto("m60", ProtoId::weapon), 1);
			inventory.add(findProto("pulse_rifle", ProtoId::weapon), 1);

			inventory.add(findProto("leather_armour", ProtoId::armour), 1);
			inventory.add(findProto("power_armour", ProtoId::armour), 1);
			inventory.add(findProto("metal_armour", ProtoId::armour), 1);
			inventory.add(findProto("mutant_armour", ProtoId::armour), 1);
			inventory.add(findProto("ghoul_armour", ProtoId::armour), 1);
			
			inventory.add(findProto("762mm", ProtoId::ammo), 250);
			inventory.add(findProto("9mm_ap", ProtoId::ammo), 100);
			inventory.add(findProto("9mm_ball", ProtoId::ammo), 100);
			inventory.add(findProto("9mm_jhp", ProtoId::ammo), 100);
			inventory.add(findProto("rocket_ap", ProtoId::ammo), 100);
			inventory.add(findProto("fusion_cell", ProtoId::ammo), 300);
		}

		respawnPC(PCIndex(0, 0), spawn_zone, inventory);
	}
		
	void SinglePlayerMode::tick(double time_diff) {
		GameMode::tick(time_diff);
	}

}
