/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor.h"
#include "game/world.h"
#include "game/sprite.h"
#include <cmath>
#include <cstdio>

namespace game {

	//TODO: different speeds for different actors
	//TODO: load speeds, sprite names, etc. from XML
	static const float s_speeds[StanceId::count + 1] = {
		25.0f,
		10.0f,
		6.0f,
		3.5f,
	};

	static const char *s_sprite_names[ActorTypeId::count][ArmourClassId::count] = {
		{	// Male
			"characters/TribalMale",
			"characters/LeatherMale",
			"characters/MetalMale",
			"characters/Environmental",
			"characters/Power",
		},
		{	// Female
			"characters/TribalFemale",
			"characters/LeatherFemale",
			"characters/MetalFemale",
			"characters/Environmental",
			"characters/Power",
		},
		{	// Ghoul
			"characters/Ghoul",
			"characters/Ghoul",
			"characters/GhoulArmour",
			"characters/Environmental",
			"characters/Power",
		},
		{ // Vault Male
			"characters/VaultMale",
			"characters/LeatherMale",
			"characters/MetalMale",
			"characters/Environmental",
			"characters/Power",
		},
		{ // Vault Female
			"characters/VaultFemale",
			"characters/LeatherFemale",
			"characters/MetalFemale",
			"characters/Environmental",
			"characters/Power",
		},
		{ // Mutant
			"characters/Mutant",
			nullptr,
			"characters/MutantArmour",
			nullptr,
			nullptr,
		},
		{ "critters/RadScorpion",		nullptr, nullptr, nullptr, nullptr, },
		{ "critters/GiantRat",			nullptr, nullptr, nullptr, nullptr, },
		{ "critters/Wolf",				nullptr, nullptr, nullptr, nullptr, },
		{ "critters/TwoHeadedBrahmin",	nullptr, nullptr, nullptr, nullptr, },
		{ "critters/MDC",				nullptr, nullptr, nullptr, nullptr, },
		{ "critters/SDC",				nullptr, nullptr, nullptr, nullptr, },
	};

	Actor::Actor(ActorTypeId::Type type_id, const float3 &pos)
		:Entity(s_sprite_names[type_id][ArmourClassId::none], pos), m_type_id(type_id) {
		//m_sprite->printSequencesInfo();
		m_anims = ActorAnims(m_sprite);

		m_issue_next_order = false;
		m_stance_id = StanceId::standing;

		m_armour_class_id = ArmourClassId::none;
		m_weapon_class_id = WeaponClassId::unarmed;
		animate(ActionId::idle);
		lookAt(float3(0, 0, 0), true);
		m_order = m_next_order = doNothingOrder();
	}

	bool Actor::isDead() const {
		return m_order.id == OrderId::die;
	}
		
	Entity *Actor::clone() const {
		return new Actor(*this);
	}
		
	void Actor::onImpact(ProjectileTypeId::Type projectile_type, float damage) {
		if(m_order.id == OrderId::die)
			return;

		DeathTypeId::Type death_id =
			projectile_type == ProjectileTypeId::plasma? DeathTypeId::melt :
			projectile_type == ProjectileTypeId::laser? DeathTypeId::melt :
			projectile_type == ProjectileTypeId::rocket? DeathTypeId::explode : DeathTypeId::normal;
		setNextOrder(dieOrder(death_id));
		issueNextOrder();
	}

	void Actor::updateArmour() {
		const ArmourDesc *desc = m_inventory.armour().desc();
		ArmourClassId::Type class_id = desc? desc->class_id : ArmourClassId::none;
		DASSERT(canEquipArmour(class_id));

		if(class_id != m_armour_class_id) {
			m_armour_class_id = class_id;
			//TODO: może nie byc miejsca na postać w nowym rozmiarze
			changeSprite(s_sprite_names[m_type_id][class_id], true);
			m_anims = ActorAnims(m_sprite);

			if(!canEquipWeapon(m_weapon_class_id)) {
				m_inventory.unequip(InventorySlotId::weapon);
				updateWeapon();
				DASSERT(canEquipWeapon(m_weapon_class_id));
			}
		}
	}

	void Actor::updateWeapon() {
		const WeaponDesc *weapon_desc = m_inventory.weapon().desc();
		WeaponClassId::Type class_id = weapon_desc? weapon_desc->class_id : WeaponClassId::unarmed;
		DASSERT(canEquipWeapon(class_id));

		if(class_id != m_weapon_class_id) {
			m_weapon_class_id = class_id;
			animate(m_action_id);
		}
	}

	bool Actor::canEquipItem(int item_id) const {
		DASSERT(item_id >= 0 && item_id < m_inventory.size());
		const Item &item = m_inventory[item_id].item;
		if(item.typeId() == ItemTypeId::weapon)
			return canEquipWeapon(Weapon(item).classId());
		else if(item.typeId() == ItemTypeId::armour)
			return canEquipArmour(Armour(item).classId());

		return true;
	}

	bool Actor::canEquipWeapon(WeaponClassId::Type class_id) const {
		DASSERT(WeaponClassId::isValid(class_id));

		for(int s = 0; s < StanceId::count; s++) {
			if(m_anims.animId(ActionId::idle, (StanceId::Type)s, class_id) == -1) {
				printf("no idle anim: %d %d\n", s, class_id);
				return false;
			}
			if(m_anims.animId(ActionId::walking, (StanceId::Type)s, class_id) == -1) {
				printf("no walk anim: %d %d\n", s, class_id);
				return false;
			}
		}

		return true;
	}

	bool Actor::canEquipArmour(ArmourClassId::Type class_id) const {
		DASSERT(ArmourClassId::isValid(class_id));
		return s_sprite_names[m_type_id][class_id] != nullptr;
	}

	bool Actor::canChangeStance() const {
		return m_anims.canChangeStance();
	}

	void Actor::think() {
		if(isDead())
			return;

		DASSERT(m_world);
		double time_delta = m_world->timeDelta();

		if(m_issue_next_order)
			issueNextOrder();

		OrderId::Type order_id = m_order.id;
		if(order_id == OrderId::do_nothing) {
			roundPos();
			m_issue_next_order = true;
		}
		else if(order_id == OrderId::move) {
			DASSERT(!m_path.empty() && m_path_pos >= 0 && m_path_pos < (int)m_path.size());
			
			float speed = s_speeds[m_order.move.run? 0 : m_stance_id + 1];
			float dist = speed * time_delta;
			m_issue_next_order = false;
			float3 new_pos;

			while(dist > 0.0001f) {
				int3 target = m_path[m_path_pos];
				if(m_world->isColliding(boundingBox() - pos() + float3(target), this, collider_tiles))
					target.y += 1;

				int3 diff = target - m_last_pos;
				float3 diff_vec(diff); diff_vec = diff_vec / length(diff_vec);
				float3 cur_pos = float3(m_last_pos) + float3(diff) * m_path_t;
				float tdist = distance(float3(target), cur_pos);

				if(tdist < dist) {
					dist -= tdist;
					m_last_pos = target;
					m_path_t = 0.0f;

					bool stop_moving = m_next_order.id != OrderId::do_nothing &&
							!(m_next_order.id == OrderId::interact && m_next_order.interact.waiting_for_move);

					if(++m_path_pos == (int)m_path.size() || stop_moving) {
						lookAt(target);
						new_pos = target;
						m_path.clear();
						m_issue_next_order = true;
						break;
					}
				}
				else {
					float new_x = cur_pos.x + diff_vec.x * dist;
					float new_z = cur_pos.z + diff_vec.z * dist;
					m_path_t = diff.x? (new_x - m_last_pos.x) / float(diff.x) : (new_z - m_last_pos.z) / float(diff.z);
					new_pos = (float3)m_last_pos + float3(diff) * m_path_t;
					lookAt(target);
					break;
				}
			}

			if(m_world->isColliding(boundingBox() + new_pos - pos(), this, collider_dynamic | collider_dynamic_nv)) {
				//TODO: response to collision
				m_issue_next_order = true;
				m_path.clear();
			}
			else
				setPos(new_pos);
		}
	}

	// sets direction
	void Actor::lookAt(const float3 &pos, bool at_once) { //TODO: rounding
		float3 cur_pos = this->pos();
		float2 dir(pos.x - cur_pos.x, pos.z - cur_pos.z);
		dir = dir / length(dir);
		m_target_angle = vectorToAngle(dir);
		if(at_once)
			setDirAngle(m_target_angle);
	}

	static float angleDist(float a, float b) {
		float diff = fabs(a - b);
		return min(diff, constant::pi * 2.0f - diff);
	}

	void Actor::nextFrame() {
		Entity::nextFrame();

		float dir_angle = dirAngle();
		if(dir_angle != m_target_angle) {
			float step = constant::pi / 4.0f;
			float new_ang1 = dir_angle + step, new_ang2 = dir_angle - step;
			if(new_ang1 < 0.0f)
				new_ang1 += constant::pi * 2.0f;
			if(new_ang2 < 0.0f)
				new_ang2 += constant::pi * 2.0f;
			float new_angle = angleDist(new_ang1, m_target_angle) < angleDist(new_ang2, m_target_angle)?
					new_ang1 : new_ang2;
			if(angleDist(dir_angle, m_target_angle) < step)
				new_angle = m_target_angle;

			setDirAngle(new_angle);
		}
	}

	void Actor::onAnimFinished() {
		if(m_order.id == OrderId::change_stance || m_order.id == OrderId::attack || m_order.id == OrderId::drop_item)
			m_issue_next_order = true;
		else if(m_order.id == OrderId::interact) {
			if(m_order.interact.mode == interact_normal) {
				m_order.target->interact(this);
			}
			m_issue_next_order = true;
		}
		else if(m_order.id == OrderId::move && m_stance_id == StanceId::crouching) { //fix for broken anims
			int seq_id = m_anims.animId(m_action_id, m_stance_id, m_weapon_class_id);
			playSequence(seq_id);
		}
	}

	void Actor::onPickupEvent() {
		//TODO: magic_hi animation when object to be picked up is high enough
		if(m_order.id == OrderId::interact) {
			DASSERT(m_order.target->entityType() == EntityId::item);
			ItemEntity *item_entity = static_cast<ItemEntity*>(m_order.target.get());
			Item item = item_entity->item();
			item_entity->remove();
			m_inventory.add(item, 1);
		}
		else if(m_order.id == OrderId::drop_item) {
			int item_id = m_order.drop_item.item_id;
			DASSERT(item_id >= 0 && item_id < m_inventory.size());
			Item item = m_inventory[item_id].item;
			m_inventory.remove(item_id, 1);
			m_world->addEntity(new ItemEntity(item, pos())); 
		}
	}
		
	void Actor::onFireEvent(const int3 &off) {
		const Weapon &weapon = m_inventory.weapon();
		if(!weapon.isValid() || m_order.id != OrderId::attack)
			return;

		//	printf("off: %d %d %d   ang: %.2f\n", off.x, off.y, off.z, dirAngle());
		float3 pos = boundingBox().center();
		pos.y = this->pos().y;
		float3 offset = asXZY(rotateVector(float2(off.x, off.z), dirAngle() - constant::pi * 0.5f), off.y);
		PProjectile projectile(new Projectile(weapon.projectileTypeId(), weapon.projectileSpeed(),
												pos + offset, m_order.attack.target_pos, this));
		m_world->spawnProjectile(std::move(projectile));
	}

	void Actor::onSoundEvent() {
	//	if(m_weapon_class_id == WeaponClassId::rifle && m_order.id == OrderId::attack)
	//		printf("Playing sound: plasma!\n");
	}

}
