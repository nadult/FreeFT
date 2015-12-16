/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/brain.h"
#include "game/actor.h"
#include "game/turret.h"
#include "game/all_orders.h"
#include "game/weapon.h"
#include "navi_map.h"

namespace game {

	Brain::Brain(World *world, EntityRef entity_ref)
		:m_world(world), m_entity_ref(entity_ref) {
		DASSERT(world);
	}
		
	int Brain::factionId() const {
		const Actor *actor = m_world->refEntity<Actor>(m_entity_ref);
		return actor? actor->factionId() : -1;
	}
		
	ThinkingEntity *Brain::entity() const {
		return m_world->refEntity<ThinkingEntity>(m_entity_ref);
	}

	Actor *Brain::actor() const {
		return m_world->refEntity<Actor>(m_entity_ref);
	}

	ActorBrain::ActorBrain(World *world, EntityRef ref)
		:Brain(world, ref), m_delay(0.0f), m_move_delay(frand() * 3.0f), m_failed_orders(0) {
	}
		
	const Weapon ActorBrain::findBestWeapon() const {
		Actor *actor = this->actor();
		if(!actor)
			return Item::dummyWeapon();;

		ActorInventory inventory = actor->inventory();

		float target_dist = distance(actor->boundingBox(), m_world->refBBox(m_target));

		inventory.unequip(ItemType::weapon);
		inventory.unequip(ItemType::ammo);

		Weapon best_weapon = inventory.dummyWeapon();
		float best_damage = best_weapon.estimateDamage();

		for(int n = 0; n < inventory.size(); n++) {
			if(inventory[n].item.type() != ItemType::weapon)
				continue;

			Weapon weapon = inventory[n].item;
			if(weapon.needAmmo() && inventory.findAmmo(weapon) == -1)
				continue;

			float damage = weapon.estimateDamage();
			
			if(target_dist < 8.0f && !weapon.hasMeleeAttack())
				continue;

			if(actor->canEquipItem(inventory[n].item) && damage > best_damage) {
				best_weapon = weapon;
				best_damage = damage;
			}
		}

		return best_weapon;
	}
		
	void ActorBrain::onImpact(DamageType::Type, float damage, const float3 &force, EntityRef source) {
		ThinkingEntity *entity = this->entity();
		if(!entity)
			return;

		if(source && entity->canSee(source)) {
			m_target = source;
			informBuddies(source);
		}
	}

	void ActorBrain::onFailed(OrderTypeId::Type) {
		m_failed_orders++;
	}
		
	void ActorBrain::findActors(int faction_id, const float3 &range, vector<EntityRef> &out) {
		ThinkingEntity *entity = this->entity();
		if(!entity)
			return;

		FBox close_prox = entity->boundingBox();
		close_prox.min -= range;
		close_prox.max += range;

		vector<ObjectRef> close_ents;
		m_world->findAll(close_ents, close_prox, {Flags::actor, m_entity_ref});
		for(int n = 0; n < (int)close_ents.size(); n++) {
			ThinkingEntity *nearby = m_world->refEntity<ThinkingEntity>(close_ents[n]);

			if(nearby && !nearby->isDying())
				if( faction_id == nearby->factionId() || (faction_id == -1 && isEnemy(nearby->factionId())) )
					if(entity->canSee(nearby->ref()))
						out.push_back(nearby->ref());
		}
	}
		
	void ActorBrain::informBuddies(EntityRef enemy) {
		Actor *actor = this->actor();
		if(!actor)
			return;

		vector<EntityRef> buddies;
		findActors(factionId(), float3(50, 20, 50), buddies);
		for(int n = 0; n < (int)buddies.size(); n++) {
			Actor *buddy = m_world->refEntity<Actor>(buddies[n]);
			if(buddy && buddy != actor) {
				ActorBrain *ai = dynamic_cast<ActorBrain*>(buddy->AI());
				if(ai && !ai->m_target)
					ai->m_target = enemy;
			}
		}
	}
		
	void ActorBrain::think() {
		ThinkingEntity *entity = this->entity();
		Actor *actor = this->actor();
		if(!entity)
			return;

		//TODO: basic visibility
		m_delay -= m_world->timeDelta();
		if(m_delay > 0.0f)
			return;

		m_delay = 0.35f;

		if(m_failed_orders >= 2)
			m_target = EntityRef();

		if(isOneOf(entity->currentOrder(), OrderTypeId::idle, OrderTypeId::track, OrderTypeId::move)) {
			if(!m_target) {
				vector<EntityRef> enemies;
				findActors(-1, float3(100, 30, 100), enemies);
			
				EntityRef best;
				float best_dist = constant::inf;

				for(int n = 0; n < (int)enemies.size(); n++) {
					float dist = distance(entity->boundingBox(), m_world->refBBox(enemies[n]));
					if(dist < best_dist) {
						best_dist = dist;
						best = enemies[n];
					}
				}

				m_failed_orders = 0;
				m_target = best;
			}

			bool can_see = entity->canSee(m_target);
			if(can_see)
				m_last_time_visible = m_world->currentTime();
			else if(m_world->currentTime() - m_last_time_visible > 5.0f)
				m_target = EntityRef();
				
			if(actor) {
				const ActorInventory &inventory = actor->inventory();
				const Weapon &weapon = inventory.weapon();
				const Weapon &best_weapon = findBestWeapon();

				if(weapon != best_weapon) {
					if(best_weapon.isDummy())
						m_world->sendOrder(new UnequipItemOrder(ItemType::weapon), m_entity_ref);
					else
						m_world->sendOrder(new EquipItemOrder(best_weapon), m_entity_ref);
					return;
				}

				if(weapon.needAmmo() && inventory.ammo().count == 0) {
					for(int n = 0; n < inventory.size(); n++)
						if(inventory[n].item.type() == ItemType::ammo && weapon.canUseAmmo(inventory[n].item)) {
							m_world->sendOrder(new EquipItemOrder(inventory[n].item), m_entity_ref);
							return;
						}
				}

				Actor *target = m_world->refEntity<Actor>(m_target);
				if(target && !target->isDying()) {
					m_move_delay = 5.0f;

					if(weapon.hasRangedAttack() && can_see) {
						AttackMode::Type mode = weapon.attackModes() & AttackMode::toFlags(AttackMode::burst)?
							AttackMode::burst : AttackMode::undefined;

						if(actor->estimateHitChance(weapon, target->boundingBox()) > 0.3)
							m_world->sendOrder(new AttackOrder(mode, m_target), m_entity_ref);
						else {
							m_world->sendOrder(new TrackOrder(m_target, 10.0f, true), m_entity_ref);
						}
					}
					else {
						FBox target_box = target->boundingBox();
						if(distance(actor->boundingBox(), target_box) <= 3.0f) {
							AttackMode::Type mode = rand() % 4 == 0? AttackMode::kick : AttackMode::undefined;
							m_world->sendOrder(new AttackOrder(mode, m_target), m_entity_ref);
						}
						else {
							m_world->sendOrder(new TrackOrder(m_target, 3.0f, true), m_entity_ref);
						}
					}
				}
				else {//TODO: change target if cannot do anything about it
					m_target = EntityRef();
				}
					

				if(!m_target && actor->currentOrder() == OrderTypeId::idle)
					tryRandomMove();
			}
			else {
				Actor *target = m_world->refEntity<Actor>(m_target);
				if(target && !target->isDying() && can_see) {
					AttackMode::Type mode = AttackMode::burst;
					if(entity->estimateHitChance(Weapon(findProto("_turret_gun", ProtoId::item_weapon)), target->boundingBox()) > 0.3)
						m_world->sendOrder(new AttackOrder(mode, m_target), m_entity_ref);
				}
				else {//TODO: change target if cannot do anything about it
					m_target = EntityRef();
				}
					


			}
		}
	}

	const float3 ActorBrain::findClosePos(float range) const {
		Actor *actor = this->actor();
		DASSERT(actor);
		float3 pos(actor->pos());

		const NaviMap *navi_map = m_world->naviMap(m_entity_ref);
		if(!navi_map)
			return pos;

		for(int iters = 0; iters < 20; iters++) {
			float3 new_pos = pos + float3((frand() - 0.5f) * range, 5.0f, (frand() - 0.5f) * range);
			if(navi_map->isReachable((int3)new_pos, (int3)pos))
				return new_pos;
		}

		return pos;
	}

	
	void ActorBrain::tryRandomMove() {
		Actor *actor = this->actor();
		DASSERT(actor);
		m_move_delay -= m_delay;

		if(m_move_delay < 0.0f) {
			vector<ObjectRef> isects;
			m_world->findAll(isects, actor->boundingBox(), {Flags::trigger});
			bool on_spawn_zone = false;
			for(auto &isect : isects) {
				const Trigger *trigger = m_world->refEntity<Trigger>(isect);
				if(trigger && trigger->classId() == TriggerClassId::spawn_zone)
					on_spawn_zone = true;
			}

			float range = rand() % 2 && !on_spawn_zone? 5.0f + frand() * 25.0f : 25.0f + frand() * 150.0f;

			float3 close_pos = findClosePos(range);
			m_world->sendOrder(new MoveOrder((int3)close_pos, false), m_entity_ref);
			m_move_delay = on_spawn_zone? 0.0f : frand() * 5.0f;
		}
	}
		
	const string ActorBrain::status() const {
		const ThinkingEntity *entity = this->entity();
		if(!entity)
			return string();
		OrderTypeId::Type current_order = entity->currentOrder();
		string order = OrderTypeId::isValid(current_order)? OrderTypeId::toString(current_order) : "invalid";

		const Actor *target = m_world->refEntity<Actor>(m_target);
		string target_text = target? string("| Target:") + target->proto().actor->id : "";

		string out;
//		out += format("(%.2f %.2f %.2f)\n", entity->pos().x, entity->pos().y, entity->pos().z);
//		out += format("HP: %d | (move_time: %.2f)\n",	entity->hitPoints(), m_move_delay);
		out += format("Order:%s %s\n", order.c_str(), target_text.c_str());
//		out += m_last_message;

		return out;
	}
		
	void ActorBrain::setEnemyFactions(const vector<int> &enemies) {
		m_enemy_factions = enemies;
	}

	bool ActorBrain::isEnemy(int faction_id) const {
		return m_enemy_factions.empty()? faction_id != factionId() : isOneOf(faction_id, m_enemy_factions);
	}

}

