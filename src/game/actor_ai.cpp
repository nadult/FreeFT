/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor_ai.h"
#include "game/actor.h"

namespace game {

	ActorAI::ActorAI(PWorld world, EntityRef actor)
		:m_world(world), m_actor_ref(actor) {
	}
		
	Actor *ActorAI::actor() const {
		return m_world->refEntity<Actor>(m_actor_ref);
	}

	SimpleAI::SimpleAI(PWorld world, EntityRef ref) :ActorAI(world, ref), m_delay(0.0f), m_failed_orders(0) {
	}
		
	void SimpleAI::tryEquipItems() {
		Actor *actor = this->actor();
		if(!actor)
			return;

		ActorInventory &inventory = actor->inventory();

		float target_dist = distance(actor->boundingBox(), m_world->refBBox(m_target));

		inventory.unequip(ItemType::weapon);
		//TODO: use orders for equipping / unequipping

		Weapon best_weapon = inventory.dummyWeapon();
		float best_damage = best_weapon.estimateDamage();
		int best_id = -1;

		for(int n = 0; n < inventory.size(); n++) {
			if(inventory[n].item.type() != ItemType::weapon)
				continue;

			Weapon weapon = inventory[n].item;
			float damage = weapon.estimateDamage();
			
			if(target_dist < 8.0f && !weapon.hasMeleeAttack())
				continue;

			if(actor->canEquipItem(n) && damage > best_damage) {
				best_weapon = weapon;
				best_damage = damage;
				best_id = n;
			}
		}

		if(best_id != -1)
			inventory.equip(best_id);
	}
		
	void SimpleAI::onImpact(DamageType::Type, float damage, const float3 &force, EntityRef source) {
		Actor *actor = this->actor();
		if(!actor)
			return;

		if(source && actor->canSee(source)) {
			m_target = source;
			informBuddies(source);
		}
	}

	void SimpleAI::onFailed(OrderTypeId::Type) {
		m_failed_orders++;
	}
		
	void SimpleAI::findActors(int faction_id, const float3 &range, vector<EntityRef> &out) {
		Actor *actor = this->actor();
		if(!actor)
			return;

		FBox close_prox = actor->boundingBox();
		close_prox.min -= range;
		close_prox.max += range;

		vector<ObjectRef> close_ents;
		m_world->findAll(close_ents, close_prox, {Flags::actor, m_actor_ref});
		for(int n = 0; n < (int)close_ents.size(); n++) {
			Actor *nearby = m_world->refEntity<Actor>(close_ents[n]);

			if(nearby && (faction_id == nearby->factionId() || nearby->factionId() != actor->factionId()) && !nearby->isDying())
				if(actor->canSee(nearby->ref()))
					out.push_back(nearby->ref());
		}
	}
		
	void SimpleAI::informBuddies(EntityRef enemy) {
		Actor *actor = this->actor();
		if(!actor)
			return;

		vector<EntityRef> buddies;
		findActors(actor->factionId(), float3(50, 20, 50), buddies);
		for(int n = 0; n < (int)buddies.size(); n++) {
			Actor *buddy = m_world->refEntity<Actor>(buddies[n]);
			if(buddy && buddy != actor) {
				SimpleAI *ai = dynamic_cast<SimpleAI*>(buddy->AI());
				if(ai && !ai->m_target)
					ai->m_target = enemy;
			}
		}
	}

	void SimpleAI::think() {
		Actor *actor = this->actor();
		if(!actor)
			return;

		//TODO: basic visibility
		m_delay -= m_world->timeDelta();
		if(m_delay > 0.0f)
			return;
		m_delay = 0.35f;

		if(m_failed_orders >= 2)
			m_target = EntityRef();

		if(actor->currentOrder() == OrderTypeId::idle || actor->currentOrder() == OrderTypeId::track) {
			tryEquipItems();

			if(!m_target) {
				vector<EntityRef> enemies;
				findActors(-1, float3(100, 30, 100), enemies);
			
				EntityRef best;
				float best_dist = constant::inf;

				for(int n = 0; n < (int)enemies.size(); n++) {
					float dist = distance(actor->boundingBox(), m_world->refBBox(enemies[n]));
					if(dist < best_dist) {
						best_dist = dist;
						best = enemies[n];
					}
				}

				m_failed_orders = 0;
				m_target = best;
			}

			bool can_see = actor->canSee(m_target);
			if(can_see)
				m_last_time_visible = m_world->currentTime();
			else if(m_world->currentTime() - m_last_time_visible > 5.0f)
				m_target = EntityRef();

			Actor *target = m_world->refEntity<Actor>(m_target);
			if(target && !target->isDying()) {
				const Weapon &weapon = actor->inventory().weapon();

				if(weapon.hasRangedAttack() && can_see) {
					AttackMode::Type mode = weapon.attackModes() & AttackMode::toFlags(AttackMode::burst)?
						AttackMode::burst : AttackMode::undefined;

					if(actor->estimateHitChance(weapon, target->boundingBox()) > 0.3)
						m_world->sendOrder(new AttackOrder(mode, m_target), m_actor_ref);
					else {
						m_world->sendOrder(new TrackOrder(m_target, 10.0f, true), m_actor_ref);
					}
				}
				else {
					FBox target_box = target->boundingBox();
					if(distance(actor->boundingBox(), target_box) <= 3.0f) {
						AttackMode::Type mode = rand() % 4 == 0? AttackMode::kick : AttackMode::undefined;
						m_world->sendOrder(new AttackOrder(mode, m_target), m_actor_ref);
					}
					else {
						m_world->sendOrder(new TrackOrder(m_target, 3.0f, true), m_actor_ref);
					}
				}
			}
			else //TODO: change target if cannot do anything about it
				m_target = EntityRef();
		}

	}

}

