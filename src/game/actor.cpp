/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor.h"
#include "game/sprite.h"
#include "game/projectile.h"
#include "game/tile.h"
#include "game/world.h"
#include "sys/xml.h"
#include "sys/data_sheet.h"
#include "net/socket.h"
#include <cmath>
#include <cstdio>

namespace game {

	ActorArmourProto::ActorArmourProto(const TupleParser &parser, bool is_actor)
		:ProtoImpl(parser), is_actor(is_actor) {
		ASSERT(!is_dummy);

		if(is_actor) {
			armour = "_dummy_armour";
		}
		else {
			actor = parser("actor_id");
			armour = parser("armour_id");
		}

		initAnims();
	}

	void ActorArmourProto::connect() {
		if(is_actor)
			actor = index();
		else
			actor.connect();
		armour.connect();

		for(int st = 0; st < StanceId::count; st++)
			for(int su = 0; su < SurfaceId::count; su++) {
				char name[256];
				snprintf(name, sizeof(name), "%s%s%s%s",
						st == StanceId::prone? "prone" : "stand", actor->is_heavy? "heavy" : "normal",
						armour->sound_prefix.c_str(), SurfaceId::toString(su));
				step_sounds[st][su] = SoundId(name);
			}
	}

	ActorProto::ActorProto(const TupleParser &parser) :ProtoImpl(parser, true) {
		is_heavy = toBool(parser("is_heavy"));
		float4 speed_vec = toFloat4(parser("speeds"));
		speeds[0] = speed_vec.x;
		speeds[1] = speed_vec.y;
		speeds[2] = speed_vec.z;
		speeds[3] = speed_vec.w;
	}

	Actor::Actor(Stream &sr) :EntityImpl(sr), m_actor(*m_proto.actor) {
	   	sr.unpack(m_target_angle, m_action_id, m_stance_id, m_issue_next_order);

		sr >> m_order >> m_next_order;
		m_burst_mode = sr.decodeInt();
		m_burst_off = net::decodeInt3(sr);

		sr >> m_path_t;
		m_path_pos = sr.decodeInt();
		m_last_pos = net::decodeInt3(sr);
		int3 prev = m_last_pos;
		int path_size = sr.decodeInt();
		m_path.resize(path_size);
		for(int i = 0; i < path_size; i++) {
			m_path[i] = net::decodeInt3(sr) + prev;
			prev = m_path[i];
		}

		m_inventory.load(sr);
	}

	Actor::Actor(const XMLNode &node) :EntityImpl(node), m_actor(*m_proto.actor) {
		initialize();
	}

	Actor::Actor(const Proto &proto, const float3 &pos)
		:EntityImpl(proto), m_actor(*m_proto.actor) {
		initialize();
		setPos(pos);
	}

	XMLNode Actor::save(XMLNode &parent) const {
		XMLNode node = EntityImpl::save(parent);
		return node;
	}

	void Actor::save(Stream &sr) const {
		EntityImpl::save(sr);
	   	sr.pack(m_target_angle, m_action_id, m_stance_id, m_issue_next_order);
		sr << m_order << m_next_order;
		sr.encodeInt(m_burst_mode);
		net::encodeInt3(sr, m_burst_off);

		sr << m_path_t;
		sr.encodeInt(m_path_pos);
		net::encodeInt3(sr, m_last_pos);
		sr.encodeInt(m_path.size());
		int3 prev = m_last_pos;
		for(int i = 0; i < (int)m_path.size(); i++) {
			net::encodeInt3(sr, m_path[i] - prev);
			prev = m_path[i];
		}

		m_inventory.save(sr);
	}

	void Actor::initialize() {
		m_issue_next_order = false;
		m_stance_id = StanceId::standing;

		animate(ActionId::idle);
		m_target_angle = dirAngle();
		m_order = m_next_order = doNothingOrder();
		m_burst_mode = false; //TODO: proper serialize
	}

	SurfaceId::Type Actor::surfaceUnder() const {
		DASSERT(world());

		//TODO: make it more robust
		FBox box_under = boundingBox();
		box_under.max.y = box_under.min.y;
		box_under.min.y -= 2.0f;
		const Tile *tile = refTile(world()->findAny(box_under, nullptr, collider_tiles));
		return tile? tile->surfaceId() : SurfaceId::unknown;
	}

	bool Actor::isDead() const {
		return m_order.id == OrderId::die;
	}
		
	void Actor::onImpact(int projectile_type, float damage) {
		if(m_order.id == OrderId::die)
			return;

		DeathTypeId::Type death_id = DeathTypeId::normal;
		/*
		DeathTypeId::Type death_id =
			projectile_type == ProjectileTypeId::plasma? DeathTypeId::melt :
			projectile_type == ProjectileTypeId::laser? DeathTypeId::melt :
			projectile_type == ProjectileTypeId::rocket? DeathTypeId::explode : DeathTypeId::normal;*/
		setNextOrder(dieOrder(death_id));
		issueNextOrder();
	}
		
	Actor::Actor(const Actor &rhs, const Proto &new_proto) :EntityImpl(new_proto), m_actor(*m_proto.actor) {
		initialize();
		setPos(rhs.pos());
		m_inventory = rhs.m_inventory;
		setDirAngle(rhs.dirAngle());
		m_target_angle = rhs.m_target_angle;
		m_stance_id = rhs.m_stance_id;
		animate(ActionId::idle);
	}

	static ProtoIndex findActorArmour(const Proto &actor, const Armour &armour) {
		return armour.isDummy()? actor.index() :
			findProto(actor.id + ":" + armour.id(), ProtoId::actor_armour);
	}

	void Actor::updateArmour() {
		ProtoIndex proto_idx = findActorArmour(m_actor, m_inventory.armour());

		if(proto_idx) {
			const Proto &new_proto = getProto(proto_idx);
			replaceMyself(PEntity(new Actor(*this, new_proto)));
		}
	}

	bool Actor::canEquipItem(int item_id) const {
		DASSERT(item_id >= 0 && item_id < m_inventory.size());
		const Item &item = m_inventory[item_id].item;
		if(item.type() == ItemType::weapon)
			return canEquipWeapon(Weapon(item).classId());
		else if(item.type() == ItemType::armour) {
			return (bool)findActorArmour(m_actor, m_inventory.armour());
		}

		return true;
	}

	bool Actor::canEquipWeapon(WeaponClassId::Type class_id) const {
		DASSERT(WeaponClassId::isValid(class_id));

		//TODO: fix this completely:
		//- when changing stance and cannot equip item in new stance: unequip

		if(m_proto.animId(ActionId::idle, m_stance_id, class_id) == -1) {
			printf("no idle anim: s:%s c:%s\n",
					StanceId::toString(m_stance_id), WeaponClassId::toString(class_id));
			return false;
		}
		if(m_proto.animId(ActionId::walking, m_stance_id, class_id) == -1) {
			printf("no walk anim: %d %d\n", m_stance_id, class_id);
			return false;
		}

		return true;
	}

	bool Actor::canChangeStance() const {
		return m_proto.canChangeStance();
	}

	void Actor::think() {
		if(isDead())
			return;
		
		double time_delta = timeDelta();
		DASSERT(world());

		if(m_issue_next_order)
			issueNextOrder();

		OrderId order_id = m_order.id;
		if(order_id == OrderId::do_nothing) {
			roundPos();
			m_issue_next_order = true;
		}
		else if(order_id == OrderId::move) {
			DASSERT(!m_path.empty() && m_path_pos >= 0 && m_path_pos < (int)m_path.size());
			
			float speed = m_actor.speeds[m_order.move.run? 0 : m_stance_id + 1];
			float dist = speed * time_delta;
			m_issue_next_order = false;
			float3 new_pos;

			while(dist > 0.0001f) {
				int3 target = m_path[m_path_pos];
				if(world()->findAny(boundingBox() - pos() + float3(target), this, collider_tiles))
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

			if(world()->findAny(boundingBox() + new_pos - pos(), this, collider_dynamic | collider_dynamic_nv)) {
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

	void Actor::nextFrame() {
		Entity::nextFrame();

		setDirAngle(blendAngles(dirAngle(), m_target_angle, constant::pi / 4.0f));

		if(m_burst_mode && m_order.id == OrderId::attack) {
			m_burst_mode++;
			fireProjectile(m_burst_off, (float3)m_order.attack.target_pos, m_inventory.weapon(), 0.05f);
			if(m_burst_mode > 15)
				m_burst_mode = 0;
		}
	}

	void Actor::onAnimFinished() {
		m_burst_mode = false;
		
		if(m_order.id == OrderId::change_stance || m_order.id == OrderId::attack || m_order.id == OrderId::drop_item)
			m_issue_next_order = true;
		else if(m_order.id == OrderId::interact) {
			if(m_order.interact.mode == interact_normal && !isClient()) {
				Entity *target = refEntity(m_order.target);
				if(target)
					target->interact(this);
			}
			m_issue_next_order = true;
		}
		else if(m_order.id == OrderId::equip_item || m_order.id == OrderId::unequip_item) {
			handleEquipOrder(m_order);
			m_issue_next_order = true;
		}
		else if(m_order.id == OrderId::move && m_stance_id == StanceId::crouching) { //fix for broken anims
			animate(m_action_id);
		}
	}

	void Actor::handleEquipOrder(const Order &order) {
		ItemType::Type changed_item = ItemType::invalid;

		if(order.id == OrderId::equip_item) {
			int item_id = order.equip_item.item_id;

			if(m_inventory.isValidId(item_id) && canEquipItem(item_id)) {
				//TODO: reloading ammo
				changed_item = m_inventory[item_id].item.type();
				if(!m_inventory.equip(item_id))
					changed_item = ItemType::invalid;
			}
				
		}
		else if(order.id == OrderId::unequip_item) {
			ItemType::Type type = order.unequip_item.item_type;
			if(m_inventory.unequip(type) != -1)
				changed_item = type;
		}
			
		if(changed_item == ItemType::armour)
			updateArmour();
	}

	void Actor::onPickupEvent() {
		if(isClient())
			return;

		//TODO: magic_hi animation when object to be picked up is high enough
		if(m_order.id == OrderId::interact) {
			Entity *target = refEntity(m_order.target);
			if(target && target->entityType() == EntityId::item) {
				ItemEntity *item_entity = static_cast<ItemEntity*>(target);
				Item item = item_entity->item();
				int count = item_entity->count();
				//TODO: what will happen if two actors will pickup at the same time?

				item_entity->remove();
				m_inventory.add(item, count);
			}
		}
		else if(m_order.id == OrderId::drop_item) {
			int item_id = m_order.drop_item.item_id;
			DASSERT(item_id >= 0 && item_id < m_inventory.size());
			Item item = m_inventory[item_id].item;
			int count = m_inventory[item_id].count;
			if(item.type() != ItemType::ammo)
				count = 1;

			m_inventory.remove(item_id, count);
			addEntity(new ItemEntity(item, count, pos())); 
		}
	}
		
	void Actor::onFireEvent(const int3 &off) {
		const Weapon &weapon = m_inventory.weapon();
		if(m_order.id != OrderId::attack || isClient())
			return;
		AttackMode::Type mode = m_order.attack.mode;

		if(mode != AttackMode::single && mode != AttackMode::burst)
			return;

		if(mode == AttackMode::burst) {
			m_burst_mode = 1;
			m_burst_off = off;
		}
		else
			fireProjectile(off, m_order.attack.target_pos, weapon, 0.0f);
	}
		
	void Actor::fireProjectile(const int3 &off, const float3 &target, const Weapon &weapon, float random_val) {
		//	printf("off: %d %d %d   ang: %.2f\n", off.x, off.y, off.z, dirAngle());
		float3 pos = boundingBox().center();
		pos.y = this->pos().y;
		float3 offset = asXZY(rotateVector(float2(off.x, off.z), actualDirAngle() - constant::pi * 0.5f), off.y);

		float3 dir = float3(target) - pos;
		float len = length(dir);
		dir *= 1.0f / len;
		float2 horiz = angleToVector(vectorToAngle(float2(dir.x, dir.z)) + frand() * constant::pi * random_val);
		dir.x = horiz.x; dir.z = horiz.y;
		dir.y += frand() * random_val;
		dir *= 1.0f / length(dir); //TODO: yea.. do this properly

		if( const ProjectileProto *proj_proto = weapon.projectileProto() )
			addEntity(new Projectile(*proj_proto, pos + offset, actualDirAngle(), pos + dir * len, makeRef()));
	}

	void Actor::onStepEvent(bool left_foot) {
		if(isServer())
			return;
		DASSERT(world());

		SurfaceId::Type standing_surface = surfaceUnder();
		world()->playSound(m_proto.step_sounds[m_stance_id][standing_surface], pos());
	}

	void Actor::onSoundEvent() {
		if(isServer())
			return;

		if(m_order.id == OrderId::attack) {
			const Weapon &weapon = m_inventory.weapon();

			//TODO: select firing mode in attack order
			world()->playSound(m_burst_mode?
						weapon.soundId(WeaponSoundType::fire_burst) :
						weapon.soundId(WeaponSoundType::fire_single), pos());
		}
	}

}
