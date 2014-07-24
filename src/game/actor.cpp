/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/actor.h"
#include "game/sprite.h"
#include "game/projectile.h"
#include "game/tile.h"
#include "game/world.h"
#include "game/weapon.h"
#include "game/game_mode.h"
#include "game/all_orders.h"
#include "sys/xml.h"
#include "sys/data_sheet.h"
#include "net/socket.h"
#include <cmath>
#include <cstdio>
#include "sys/profiler.h"
#include "gfx/scene_renderer.h"

//#define DEBUG_SHOOTING

namespace game {

	Actor::Actor(Stream &sr)
	  :EntityImpl(sr), m_actor(*m_proto.actor) {
		m_inventory.setDummyWeapon(Weapon(*m_actor.punch_weapon));

		u8 flags;
		sr.unpack(flags, m_stance, m_action);
		m_hit_points = sr.decodeInt();
		m_sound_variation = sr.decodeInt();
		m_client_id = sr.decodeInt();
		m_faction_id = sr.decodeInt();

		if(flags & 1)
			sr >> m_target_angle;
		else
			m_target_angle = dirAngle();
		sr >> m_inventory;
	}

	Actor::Actor(const XMLNode &node)
	  :EntityImpl(node), m_actor(*m_proto.actor), m_inventory(node.child("inventory")), m_stance(Stance::stand),
	      m_target_angle(dirAngle()), m_client_id(-1) {
		m_inventory.setDummyWeapon(Weapon(*m_actor.punch_weapon));
		m_sound_variation = node.intAttrib("sound_variation") % m_actor.sounds.size();
		m_faction_id = node.intAttrib("faction_id");
		m_hit_points = m_actor.hit_points;
		animate(Action::idle);
	}

	Actor::Actor(const Proto &proto, Stance::Type stance)
		:EntityImpl(proto), m_actor(*m_proto.actor), m_stance(stance), m_target_angle(dirAngle()), m_client_id(-1) {
		m_inventory.setDummyWeapon(Weapon(*m_actor.punch_weapon));
		m_sound_variation = rand() % m_actor.sounds.size();
		m_faction_id = 0;
		m_hit_points = m_actor.hit_points;
		animate(Action::idle);
	}

	static ProtoIndex findActorArmour(const Proto &actor, const Armour &armour) {
		return armour.isDummy()? actor.index() :
			findProto(actor.id + ":" + armour.id(), ProtoId::actor_armour);
	}

	static const Proto &getActorArmour(const Proto &actor, ActorInventory &inventory) {
		ProtoIndex out = actor.index();

		if(inventory.isEquipped(ItemType::armour)) {
			ProtoIndex armour_proto = findActorArmour(actor, inventory.armour());
			if(armour_proto)
				out = armour_proto;
			else
				inventory.unequip(ItemType::armour);
		}

		const ActorArmourProto &proto = static_cast<const ActorArmourProto&>(getProto(out));
		if(inventory.isEquipped(ItemType::weapon) && !proto.canEquipWeapon(inventory.weapon().classId()))
			inventory.unequip(ItemType::weapon);

		return proto;
	}

	Actor::Actor(const Proto &proto, ActorInventory &inv) :EntityImpl(getActorArmour(proto, inv)),
		  m_actor(*m_proto.actor), m_stance(Stance::stand), m_inventory(inv), m_target_angle(dirAngle()), m_client_id(-1) {
		m_inventory.setDummyWeapon(Weapon(*m_actor.punch_weapon));
		m_sound_variation = rand() % m_actor.sounds.size();
		m_faction_id = 0;
		m_hit_points = m_actor.hit_points;
		animate(Action::idle);
	}


	Actor::Actor(const Actor &rhs, const Proto &new_proto) :Actor(new_proto, rhs.m_stance) {
		setPos(rhs.pos());
		setDirAngle(m_target_angle = rhs.dirAngle());
		m_inventory = rhs.m_inventory;
		m_faction_id = rhs.m_faction_id;
		m_sound_variation = rhs.m_sound_variation;
		m_hit_points = rhs.m_hit_points;
	}
	
	XMLNode Actor::save(XMLNode &parent) const {
		XMLNode node = EntityImpl::save(parent);
		node.addAttrib("faction_id", m_faction_id);
		node.addAttrib("sound_variation", m_sound_variation);
		if(!m_inventory.isEmpty())
			m_inventory.save(node.addChild("inventory"));
		return node;
	}

	void Actor::save(Stream &sr) const {
		EntityImpl::save(sr);
		u8 flags =	(m_target_angle != dirAngle()? 1 : 0);

		sr.pack(flags, m_stance, m_action);
		sr.encodeInt(m_hit_points);
		sr.encodeInt(m_sound_variation);
		sr.encodeInt(m_client_id);
		sr.encodeInt(m_faction_id);

		if(flags & 1)
			sr << m_target_angle;
		sr << m_inventory;
	}
		
	Flags::Type Actor::flags() const {
		return Flags::actor | Flags::dynamic_entity | (isDead()? (Flags::Type)0 : Flags::colliding);
	}
		
	const FBox Actor::boundingBox() const {
		int3 bbox_size = sprite().bboxSize();
		if(m_stance == Stance::crouch)
			bbox_size.y = 6;
		if(m_stance == Stance::prone || isOneOf(m_action, Action::fallen_back, Action::fallen_forward))
			bbox_size.y = 2;
		if(isDead())
			bbox_size.y = 0;

		return FBox(pos(), pos() + float3(bbox_size));
	}

	SurfaceId::Type Actor::surfaceUnder() const {
		//TODO: make it more robust
		FBox box_under = boundingBox();
		box_under.max.y = box_under.min.y;
		box_under.min.y -= 1.0f;
		const Tile *tile = nullptr;
		for(int i = 0; i < 2 && !tile; i++) {
			tile = refTile(findAny(box_under, Flags::tile | Flags::colliding));
			box_under.min.y -= 1.0f;
			box_under.max.y -= 1.0f;
		}

		return tile? tile->surfaceId() : SurfaceId::unknown;
	}
		
	float Actor::dodgeChance(DamageType::Type type, float damage) const {
		return	type == DamageType::bludgeoning ||
				type == DamageType::slashing ||
				type == DamageType::piercing? 0.2f : 0.0f;
	}

	float Actor::fallChance(DamageType::Type type, float damage, const float3 &force_vec) const {
		float force = length(force_vec) * (m_action == Action::walk? 1.25f : m_action == Action::run? 1.5f : 1.0f) * 0.2f - 0.5f;
		if(type == DamageType::bludgeoning || type == DamageType::explosive)
			force *= 1.25f;

		if(force <= 0.0f)
			return 0.0f;
		// A guy in power armour shouldn't be so easy to knock down
		return pow(force / (force + 1.0f), 2.0f);
	}
	
	float Actor::interruptChance(DamageType::Type type, float damage, const float3 &force_vec) const {
		//TODO: work on it
		if(damage < m_actor.hit_points * 0.05f && length(force_vec) < 1.0f)
			return 0.25f;
		if(damage < m_actor.hit_points * 0.1f && length(force_vec) < 2.0f)
			return 0.4f;
		return 0.5f;
	}

	DeathId::Type Actor::deathType(DamageType::Type damage_type, float damage, const float3 &force_vec) const {
		float damage_rate = damage / float(m_actor.hit_points);
		
		if(damage_type == DamageType::plasma || damage_type == DamageType::laser)
			return DeathId::melt;
		if(damage_type == DamageType::electric)
			return DeathId::electrify;
		if(damage_type == DamageType::fire)
			return DeathId::fire;
		if(damage_type == DamageType::explosive && damage_rate > 0.3f)
			return DeathId::explode;
		if(damage_type == DamageType::slashing && damage_rate > 0.2f)
			return DeathId::cut_in_half;
		if(damage_type == DamageType::bullet && damage_rate > 0.2f)
			return DeathId::big_hole;

		//TODO: record recent damage and include this damage in computation
		//TODO: riddled & big_hole

		return DeathId::normal;	
	}

	void Actor::onImpact(DamageType::Type damage_type, float damage, const float3 &force, EntityRef source) {
		float damage_res = clamp(1.0f - m_inventory.armour().proto().damage_resistance, 0.0f, 1.0f);
		damage *= damage_res;

		if(isDying()) {
			m_hit_points -= int(damage);
			return;
		}
			
		GetHitOrder *current = m_order && m_order->typeId() == OrderTypeId::get_hit? static_cast<GetHitOrder*>(m_order.get()) : nullptr;

		bool is_fallen = current && (current->mode == GetHitOrder::Mode::fall || current->mode == GetHitOrder::Mode::fallen);

		//TODO: randomization provided by world class
		//TODO: synchronization problems in multiplayer, seed should be passed with damage
		bool will_dodge = !is_fallen && random() <= dodgeChance(damage_type, damage);
		bool will_fall = !will_dodge && random() <= fallChance(damage_type, damage, force);
		bool will_interrupt = random() <= interruptChance(damage_type, damage, force);
		
		if(!will_dodge)
			m_hit_points -= int(damage);

		if(m_hit_points <= 0.0f) {
			DeathId::Type death_id = deathType(damage_type, damage, force);
			setOrder(new DieOrder(death_id), true);
			onKill(ref(), source);
		}
		else {
			if(will_fall) {
				float fall_time = (damage / float(m_actor.hit_points)) * length(force) * random();

				if(is_fallen && current)
					current->fall_time += fall_time;
				else
					setOrder(new GetHitOrder(force, fall_time), true);
			}
			else if(!current) {
				OrderTypeId::Type current_type_id = m_order? m_order->typeId() : OrderTypeId::invalid;

				if(current_type_id != OrderTypeId::change_stance && (current_type_id == OrderTypeId::idle || random() > 0.5f))
					setOrder(new GetHitOrder(will_dodge), true);
				else if(!will_dodge)
					replicateSound(m_actor.sounds[m_sound_variation].hit, pos());
			}
		}

		//TODO: maybe this should be called first?
		ThinkingEntity::onImpact(damage_type, damage, force, source);	
	}
		
	void Actor::updateArmour() {
		ProtoIndex proto_idx = findActorArmour(m_actor, m_inventory.armour());

		if(proto_idx) {
			const Proto &new_proto = getProto(proto_idx);
			replaceMyself(PEntity(new Actor(*this, new_proto)));
		}
	}

	WeaponClass::Type Actor::equippedWeaponClass() const {
		return m_inventory.weapon().classId();
	}
		
	static const float feel_distance = 30.0;
	static const float max_distance = 300.0;

	bool Actor::canSee(EntityRef target_ref, bool simple_test) {
		const Entity *target = refEntity(target_ref);
		if(!target || isDead())
			return false;

		const FBox &box = boundingBox();
		const FBox &target_box = target->boundingBox();
		float dist = distance(box, target_box);
		float3 eye_pos = asXZY(box.center().xz(), box.min.y + box.height() * 0.75f);

		if(dist > max_distance)
			return false;

		if(dist > feel_distance && !isInsideFrustum(eye_pos, asXZY(dir(), 0.0f), 0.1f, target_box))
			return false;
		
		if(simple_test)
			return true;

		return world()->isVisible(eye_pos, target_ref, ref(), target->typeId() == EntityId::actor? 4 : 3);
	}

	//TODO: move to actorinventory (part at least)
	bool Actor::canEquipItem(const Item &item) const {
		if(item.type() == ItemType::weapon)
			return m_proto.canEquipWeapon(Weapon(item).classId());
		else if(item.type() == ItemType::armour)
			return (bool)findActorArmour(m_actor, Armour(item));
		else if(item.type() == ItemType::ammo)
			return m_inventory.weapon().proto().ammo_class_id == Ammo(item).classId();

		return false;
	}

	bool Actor::canChangeStance() const {
		return m_proto.canChangeStance();
	}

	bool Actor::setOrder(POrder &&order, bool force) {
		if(order->typeId() == OrderTypeId::look_at) {
			if((m_order && m_order->typeId() != OrderTypeId::idle) || !m_following_orders.empty())
				return false;
		}

		if(m_order && order->typeId() == m_order->typeId()) {
			if(order->typeId() == OrderTypeId::change_stance) {
				ChangeStanceOrder *current_order = static_cast<ChangeStanceOrder*>(m_order.get());
				ChangeStanceOrder *new_order = static_cast<ChangeStanceOrder*>(order.get());

				current_order->m_target_stance = new_order->m_target_stance;
				order.reset();
			}
		}

		if(order && m_order && m_order->typeId() != OrderTypeId::change_stance)
			m_order->cancel();

		return ThinkingEntity::setOrder(std::move(order), force);
	}

	void Actor::think() {
		ThinkingEntity::think();
		m_move_vec = float3(0, 0, 0);
	}

	// sets direction
	void Actor::lookAt(const float3 &pos, bool at_once) { //TODO: rounding
		float3 center = boundingBox().center();
		float2 dir(pos.x - center.x, pos.z - center.z);
		float len = length(dir);
		if(len < constant::epsilon)
			return;

		dir = dir / len;
		m_target_angle = vectorToAngle(dir);
		if(at_once)
			setDirAngle(m_target_angle);
	}

	void Actor::nextFrame() {
		setDirAngle(blendAngles(dirAngle(), m_target_angle, constant::pi / 4.0f));
		ThinkingEntity::nextFrame();
	}
		
	bool Actor::isDying() const {
		return m_order && m_order->typeId() == OrderTypeId::die;
	}
		
	bool Actor::isDead() const {
		return m_order && m_order->typeId() == OrderTypeId::die &&
					static_cast<DieOrder*>(m_order.get())->m_is_dead;
	}

	bool Actor::animate(Action::Type action) {
		DASSERT(!Action::isSpecial(action));

		int anim_id = m_proto.animId(action, m_stance, equippedWeaponClass());
		if(anim_id == -1 && Action::isNormal(action))
			anim_id = m_proto.animId(action, m_stance, WeaponClass::unarmed);

		if(anim_id == -1)
			return false;

		m_action = action;
		playSequence(anim_id);

		return true;
	}
		
	bool Actor::animateDeath(DeathId::Type death_type) {
		int anim_id = m_proto.deathAnimId(death_type);
		if(anim_id == -1)
			return false;
		playSequence(anim_id);
		m_action = Action::death;
		return true;
	}

	const Path Actor::currentPath() const {
		if(m_order && m_order->typeId() == OrderTypeId::move)
			return Path(static_cast<MoveOrder*>(m_order.get())->m_path);

		return Path();
	}
		
	const float3 Actor::estimateMove(float time_advance) const {
		return m_move_vec * time_advance;
	}

	Actor::FollowPathResult Actor::followPath(const Path &path, PathPos &path_pos, bool run) {
		float speed = m_actor.speeds[run && m_stance == Stance::stand? 3 : m_stance];
		float dist = speed * timeDelta();
		float max_step = 1.0f;
		bool has_finished = false;

		while(dist > 0.0f && !has_finished) {
			float step = min(dist, max_step);
			dist -= step;
			if(path.follow(path_pos, step))
				has_finished = true;

			float3 new_pos = path.pos(path_pos);
			FBox bbox = (FBox)enclosingIBox(boundingBox() + new_pos - pos());

			ObjectRef tile_ref = findAny(bbox, Flags::tile | Flags::colliding);
			if(tile_ref) {
				const FBox tile_bbox = refBBox(tile_ref);
				float diff = bbox.min.y - tile_bbox.max.y;
				if(diff > 1.0f) {
					fixPosition();
					return FollowPathResult::collided;
				}
				if(diff > 0.0f) {
					new_pos.y += diff;
					bbox.min.y += diff;
					bbox.max.y += diff;
				}
			}
			
			bbox.min += float3(0.05, 0.05, 0.05);
			bbox.max -= float3(0.05, 0.05, 0.05);

			if(findAny(bbox, {Flags::dynamic_entity | Flags::colliding, ref()})) {
				fixPosition();
				//TODO: response to collision
				return FollowPathResult::collided;
			}
				
			m_move_vec = (new_pos - pos()) * speed / step;
			lookAt(new_pos + bboxSize() * 0.5f);
			setPos(new_pos);
		}

		if(has_finished)
			fixPosition();

		return has_finished? FollowPathResult::finished : FollowPathResult::moved;
	}
		
	void Actor::fixPosition() {
		int3 new_pos(pos() + float3(0.5f, -0.5f, 0.5f));
		setPos(new_pos);

		for(int i = 0; i < 2 && findAny(boundingBox(), Flags::tile | Flags::colliding); i++)
			setPos(pos() + float3(0.0f, 1.0f, 0.0f));
	}
	
	void Actor::fireProjectile(const FBox &target_box, const Weapon &weapon, float randomness) {
		if(isClient())
			return;

		Segment best_ray = computeBestShootingRay(target_box, weapon);

		if(randomness > 0.0f) {
			float3 dir = perturbVector(best_ray.dir(), random(), random(), randomness);
			best_ray = Ray(best_ray.origin(), dir);
		}

#ifdef DEBUG_SHOOTING
		{
			m_aiming_lines.clear();
			m_aiming_lines.push_back(best_ray.origin());
			Intersection isect = trace(best_ray, {Flags::all | Flags::colliding, ref()});
			m_aiming_lines.push_back(best_ray.origin() + best_ray.dir() * isect.distance());
		}
#endif

		//TODO: spawned projectiles should be centered
		if( const ProjectileProto *proj_proto = weapon.projectileProto() )
			addNewEntity<Projectile>(best_ray.origin(), *proj_proto, actualDirAngle(), best_ray.dir(), ref(), weapon.proto().damage_mod);
	}
		
	void Actor::makeImpact(EntityRef target, const Weapon &weapon) {
		DASSERT(weapon.proto().impact.isValid());

		if(target) {
			const Armour &armour = m_inventory.armour();
			float damage_mod = weapon.proto().damage_mod * armour.proto().melee_mod;
			addNewEntity<Impact>(pos(), *weapon.proto().impact, ref(), target, damage_mod);
		}
	}

	void Actor::addToRender(gfx::SceneRenderer &out, Color color) const {
		Entity::addToRender(out, color);

#ifdef DEBUG_SHOOTING
	//	for(int n = 0; n < (int)m_aiming_points.size(); n++)
	//		out.addBox(FBox(m_aiming_points[n] - float3(1,1,1) * 0.1f, m_aiming_points[n] + float3(1,1,1) * 0.1f), Color::red, true);
	//	for(int n = 0; n < (int)m_aiming_lines.size(); n+=2)
	//		out.addLine((int3)m_aiming_lines[n], (int3)m_aiming_lines[n + 1], Color::red);

		const AttackOrder *attack = m_order && m_order->typeId() == OrderTypeId::attack? static_cast<AttackOrder*>(m_order.get()) : nullptr;
		if(attack) {
			const Actor *target = const_cast<Actor*>(this)->refEntity<Actor>(attack->m_target);
			if(target) {
				FBox bounding_box = target->boundingBox() + attack->m_target_pos - target->boundingBox().center();
				out.addBox(bounding_box, Color(255, 0, 0, 100), false);
			}
		}
#endif
	}

	const FBox Actor::shootingBox(const Weapon &weapon) const {
		FBox bbox = boundingBox();
		float3 center = bbox.center();
		float3 size = bbox.size();

		if(weapon.classId() == WeaponClass::minigun) {
			size.y *= 0.4f;
		}
		else {
			center.y += size.y * 0.3f;
			size.y *= 0.4f;
		}

		return FBox(center - size * 0.5f, center + size * 0.5f);
	}

	float Actor::accuracy(const Weapon &weapon) const {
		float accuracy = weapon.proto().accuracy;
		if(m_stance == Stance::crouch)
			accuracy *= 1.25f;
		else if(m_stance == Stance::prone)
			accuracy *= 1.5f;

		//TODO: increased accuracy, when player is not rotating for some time

		return accuracy;
	}

	AttackMode::Type Actor::validateAttackMode(AttackMode::Type in_mode) const {
		AttackMode::Type mode = in_mode;
		Weapon weapon = m_inventory.weapon();

		uint modes = weapon.attackModes();
		if(mode != AttackMode::undefined)	
			modes &= AttackMode::toFlags(mode);
		mode = AttackModeFlags::getFirst(modes);
	
		if(mode == AttackMode::undefined)
			if(weapon.canKick() && in_mode == AttackMode::kick && m_actor.kick_weapon.isValid())
				return AttackMode::kick;
		return mode;
	}

}
