/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor_ai.h"
#include "game/actor.h"
#include "navi_map.h"

namespace game {

	ActorAI::ActorAI(PWorld world, EntityRef actor_ref)
		:m_world(world), m_actor_ref(actor_ref) {
		DASSERT(world);
	}
		
	int ActorAI::factionId() const {
		const Actor *actor = m_world->refEntity<Actor>(m_actor_ref);
		return actor? actor->factionId() : -1;
	}
		
	Actor *ActorAI::actor() const {
		return m_world->refEntity<Actor>(m_actor_ref);
	}

	SimpleAI::SimpleAI(PWorld world, EntityRef ref) :ActorAI(world, ref), m_delay(0.0f), m_move_delay(frand() * 3.0f), m_failed_orders(0) {
	}
		
	const Weapon SimpleAI::findBestWeapon() const {
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

			if(nearby && !nearby->isDying())
				if( faction_id == nearby->factionId() || (faction_id == -1 && isEnemy(nearby->factionId())) )
					if(actor->canSee(nearby->ref()))
						out.push_back(nearby->ref());
		}
	}
		
	void SimpleAI::informBuddies(EntityRef enemy) {
		Actor *actor = this->actor();
		if(!actor)
			return;

		vector<EntityRef> buddies;
		findActors(factionId(), float3(50, 20, 50), buddies);
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

		if(isOneOf(actor->currentOrder(), OrderTypeId::idle, OrderTypeId::track, OrderTypeId::move)) {
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
				
			const ActorInventory &inventory = actor->inventory();
			const Weapon &weapon = inventory.weapon();
			const Weapon &best_weapon = findBestWeapon();

			if(weapon != best_weapon) {
				if(best_weapon.isDummy())
					m_world->sendOrder(new UnequipItemOrder(ItemType::weapon), m_actor_ref);
				else
					m_world->sendOrder(new EquipItemOrder(best_weapon), m_actor_ref);
				return;
			}

			if(weapon.needAmmo() && inventory.ammo().count == 0) {
				for(int n = 0; n < inventory.size(); n++)
					if(inventory[n].item.type() == ItemType::ammo && weapon.canUseAmmo(inventory[n].item)) {
						m_world->sendOrder(new EquipItemOrder(inventory[n].item), m_actor_ref);
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

			if(!m_target && actor->currentOrder() == OrderTypeId::idle)
				tryRandomMove();
		}
	}

	const float3 SimpleAI::findClosePos(float range) const {
		Actor *actor = this->actor();
		DASSERT(actor);

		//TODO: use world::accessNaviMap
		const NaviMap *navi_map = m_world->naviMap((int)round(max(actor->bboxSize().x, actor->bboxSize().z)));
		DASSERT(navi_map);

		float3 pos(actor->pos());

		for(int iters = 0; iters < 20; iters++) {
			float3 new_pos = pos + float3((frand() - 0.5f) * range, 5.0f, (frand() - 0.5f) * range);
			if(navi_map->isReachable((int3)new_pos, (int3)pos)) {
				//m_last_message = format("found %d", rand());
				return new_pos;
			}
		}
		//m_last_message = format("not found %d", rand());

		return pos;
	}

	
	void SimpleAI::tryRandomMove() {
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

			float range = rand() % 2 && !on_spawn_zone? frand() * 25.0f : 25.0f + frand() * 150.0f;

			float3 close_pos = findClosePos(range);
			m_world->sendOrder(new MoveOrder((int3)close_pos, false), m_actor_ref);
			m_move_delay = on_spawn_zone? 0.0f : frand() * 5.0f;
		}
	}
		
	const string SimpleAI::status() const {
		const Actor *actor = this->actor();
		if(!actor)
			return string();
		OrderTypeId::Type current_order = actor->currentOrder();
		string order = OrderTypeId::isValid(current_order)? OrderTypeId::toString(current_order) : "invalid";
		return format("(%.2f %.2f %.2f)\nHP: %d | order: %s (time: %.2f)\n%s", actor->pos().x, actor->pos().y, actor->pos().z,
				actor->hitPoints(), order.c_str(), m_move_delay, m_last_message.c_str());
	}
		
	void SimpleAI::setEnemyFactions(const vector<int> &enemies) {
		m_enemy_factions = enemies;
	}

	bool SimpleAI::isEnemy(int faction_id) const {
		return m_enemy_factions.empty()? faction_id != factionId() : isOneOf(faction_id, m_enemy_factions);
	}

}

