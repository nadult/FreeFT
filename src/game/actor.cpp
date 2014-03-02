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

		for(int st = 0; st < Stance::count; st++)
			for(int su = 0; su < SurfaceId::count; su++) {
				char name[256];
				snprintf(name, sizeof(name), "%s%s%s%s",
						st == Stance::prone? "prone" : "stand", actor->is_heavy? "heavy" : "normal",
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
		u8 flags;
		sr >> flags;
		if(flags & 1) {
			sr >> m_order;
			updateOrderFunc();
		}
		if(flags & 2)
			sr >> m_next_order;
		if(flags & 4)
			sr >> m_target_angle;
		else
			m_target_angle = dirAngle();
		sr.unpack(m_action_id, m_stance);
		sr >> m_inventory;
	}

	Actor::Actor(const XMLNode &node) :EntityImpl(node), m_actor(*m_proto.actor), m_stance(Stance::standing), m_target_angle(dirAngle()) {
		animate(ActionId::idle);
		updateOrderFunc();
	}

	Actor::Actor(const Proto &proto, Stance::Type stance)
		:EntityImpl(proto), m_actor(*m_proto.actor), m_stance(stance), m_target_angle(dirAngle()) {
		animate(ActionId::idle);
		updateOrderFunc();
	}

	Actor::Actor(const Actor &rhs, const Proto &new_proto) :Actor(new_proto, rhs.m_stance) {
		setPos(rhs.pos());
		setDirAngle(m_target_angle = rhs.dirAngle());
		m_inventory = rhs.m_inventory;
	}
	
	XMLNode Actor::save(XMLNode &parent) const {
		XMLNode node = EntityImpl::save(parent);
		return node;
	}

	void Actor::save(Stream &sr) const {
		EntityImpl::save(sr);
		u8 flags =	(m_order? 1 : 0) |
					(m_next_order? 2 : 0) |
					(m_target_angle != dirAngle()? 4 : 0);

		sr << flags;
		if(flags & 1)
			sr << m_order;
		if(flags & 2)
			sr << m_next_order;
		if(flags & 4)
			sr << m_target_angle;
		sr.pack(m_action_id, m_stance);
		sr << m_inventory;
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
		return m_order && m_order->typeId() == OrderTypeId::die;
	}
		
	void Actor::onImpact(int projectile_type, float damage) {
		DeathTypeId::Type death_id = DeathTypeId::explode;
		/*	projectile_type == ProjectileTypeId::plasma? DeathTypeId::melt :
			projectile_type == ProjectileTypeId::laser? DeathTypeId::melt :
			projectile_type == ProjectileTypeId::rocket? DeathTypeId::explode : DeathTypeId::normal; */
		
		//TODO: immediate order cancel
		setOrder(new DieOrder(death_id));
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

		if(m_proto.animId(ActionId::idle, m_stance, class_id) == -1) {
			printf("no idle anim: s:%s c:%s\n",
					Stance::toString(m_stance), WeaponClassId::toString(class_id));
			return false;
		}
		if(m_proto.animId(ActionId::walking, m_stance, class_id) == -1) {
			printf("no walk anim: %d %d\n", m_stance, class_id);
			return false;
		}

		return true;
	}

	bool Actor::canChangeStance() const {
		return m_proto.canChangeStance();
	}
		
	void Actor::handleOrder(ActorEvent::Type event, const ActorEventParams &params) {
		if(m_order) 
			(this->*m_order_func)(m_order.get(), event, params);
	}

	bool Actor::setOrder(POrder &&order) {
		if(!world() || isClient())
			return false;

		//TODO: pass information about wheter this order can be handled
		m_next_order = std::move(order);
		if(m_order)
			m_order->cancel();

		replicate();

		return true;
	}

	void Actor::think() {
		if(isDead())
			return;
		
		double time_delta = timeDelta();
		DASSERT(world());

		if(!m_order || m_order->isFinished()) {
			if(m_order && m_order->hasFollowup())
				m_order = m_order->getFollowup();
			else
				m_order = m_next_order? std::move(m_next_order) : new IdleOrder();

			updateOrderFunc();
			handleOrder(ActorEvent::init_order);
			replicate();
		}

		handleOrder(ActorEvent::think);
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
		handleOrder(ActorEvent::next_frame);	
	}

	void Actor::onAnimFinished() {
		handleOrder(ActorEvent::anim_finished);
	}

	void Actor::onPickupEvent() {
		if(isClient())
			return;
		handleOrder(ActorEvent::pickup);
	}
		
	void Actor::onFireEvent(const int3 &off) {
		handleOrder(ActorEvent::fire, ActorEventParams{off, false});
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
			addEntity(new Projectile(*proj_proto, pos + offset, actualDirAngle(), pos + dir * len, ref()));
	}

	void Actor::onStepEvent(bool left_foot) {
		if(isServer())
			return;
		handleOrder(ActorEvent::step, ActorEventParams{ int3(), left_foot });
	}

	void Actor::onSoundEvent() {
		if(isServer())
			return;

		handleOrder(ActorEvent::sound);
	}

}
