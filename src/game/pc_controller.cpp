/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/pc_controller.h"
#include "game/character.h"
#include "game/actor.h"
#include "game/inventory.h"
#include "game/game_mode.h"

namespace game {

	PCController::PCController(World &world, const PlayableCharacter &pc)
		:m_world(world), m_pc(pc) {
		const Actor *actor = this->actor();
		m_target_stance = actor? actor->stance() : -1;
	}
		
	PCController::~PCController() { }
		
	const Actor *PCController::actor() const {
		const Actor *out = m_world.refEntity<Actor>(m_pc.entityRef());
		return out;
	}
	
	void PCController::sendOrder(game::POrder &&order) {
		m_world.sendOrder(std::move(order), m_pc.entityRef());
	}
		
	bool PCController::hasActor() const {
		return actor() != nullptr;
	}

	bool PCController::canChangeStance() const {
		return hasActor();
	}

	void PCController::setStance(Stance::Type stance) {
		DASSERT(canChangeStance());
		sendOrder(new ChangeStanceOrder(stance));
		m_target_stance = stance;
	}
		
	bool PCController::canEquipItem(const Item &item) const {
		const Actor *actor = this->actor();
		return actor && actor->canEquipItem(item);
	}
		
	void PCController::equipItem(const Item &item) {
		DASSERT(canEquipItem(item));
		sendOrder(new EquipItemOrder(item));
	}

	void PCController::unequipItem(const Item &item) {
		sendOrder(new UnequipItemOrder(item.type()));
	}

	void PCController::dropItem(const Item &item, int count) {
		sendOrder(new DropItemOrder(item, count));
	}
		
	void PCController::reload() {
		const Actor *actor = this->actor();
		if(!actor)
			return;

		const ActorInventory &inventory = actor->inventory();
		const Weapon &weapon = inventory.weapon();
		if(weapon.needAmmo() && inventory.ammo().count < weapon.maxAmmo()) {
			int item_id = inventory.find(inventory.ammo().item);
			if(item_id == -1) for(int n = 0; n < inventory.size(); n++)
				if(weapon.canUseAmmo(inventory[n].item)) {
					item_id = n;
					break;
				}
			if(item_id != -1)
				sendOrder(new EquipItemOrder(inventory[item_id].item));
		}
	}
		
		
	int PCController::classId() const {
		return m_pc.classId();
	}

	void PCController::setClassId(int class_id) {
		m_pc.setClassId(class_id);

		GameModeClient *game_mode_client = dynamic_cast<GameModeClient*>(m_world.gameMode());
		if(game_mode_client) {
			game_mode_client->setPCClassId(m_pc.character(), m_pc.classId());
		}
	}

}
