/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/thinking_entity.h"
#include "game/brain.h"
#include "game/orders/idle.h"
#include "game/weapon.h"
#include "sys/profiler.h"
#include "sys/xml.h"

using namespace gfx;


namespace game {


	ThinkingEntity::ThinkingEntity(const Sprite &sprite) :Entity(sprite) { }
	ThinkingEntity::ThinkingEntity(const ThinkingEntity&) = default;
	ThinkingEntity::~ThinkingEntity() = default;
	ThinkingEntity::ThinkingEntity(const Sprite &sprite, const XMLNode &node) :Entity(sprite, node) { }
	
	enum {
		flag_has_order = 1,
		flag_has_following_orders = 2,
	};

	ThinkingEntity::ThinkingEntity(const Sprite &sprite, Stream &sr) :Entity(sprite, sr) {
		u8 flags;
		sr >> flags;

		if(flags & flag_has_order)
			m_order.reset(Order::construct(sr));
		if(flags & flag_has_following_orders) {
			int count = sr.decodeInt();
			ASSERT(count >= 1);
			m_following_orders.resize(count);
			for(int n = 0; n < count; n++)
				m_following_orders[n].reset(Order::construct(sr));
		}

	}
		
	XMLNode ThinkingEntity::save(XMLNode& parent) const {
		return Entity::save(parent);
	}

	void ThinkingEntity::save(Stream &sr) const {
		Entity::save(sr);

		u8 flags =	(m_order? flag_has_order : 0) |
					(!m_following_orders.empty()? flag_has_following_orders : 0);
		sr << flags;

		if(flags & flag_has_order)
			sr << m_order->typeId() << *m_order;
		if(flags & flag_has_following_orders) {
			sr.encodeInt((int)m_following_orders.size());
			for(int n = 0; n < (int)m_following_orders.size(); n++)
				sr << m_following_orders[n]->typeId() << *m_following_orders[n];
		}
	}
	
	bool ThinkingEntity::setOrder(POrder &&order, bool force) {
		DASSERT(world());

		if(order) {
			if(force) {
				m_order.reset(nullptr);
				m_following_orders.insert(m_following_orders.begin(), std::move(order));
			}
			else {
				//TODO: give player possibility to clear current orders
				//TODO: fix this! why are we clearing orders?
			//	m_following_orders.clear();
				m_following_orders.emplace_back(std::move(order));
			}
		}

		replicate();

		//TODO: pass information about wheter this order can be handled
		return true;
	}

	OrderTypeId::Type ThinkingEntity::currentOrder() const {
		return m_order? m_order->typeId() : OrderTypeId::invalid;
	}
		
	void ThinkingEntity::think() {
		double time_delta = timeDelta();
		DASSERT(world());

		bool need_replicate = false;
		while(!m_order || m_order->isFinished()) {
			if(m_order && m_order->hasFollowup())
				m_order = m_order->getFollowup(); //TODO: maybe followup orders shouldnt be initialized twice?
			else {
				if(m_following_orders.empty()) {
					if(!m_order || m_order->typeId() != OrderTypeId::idle)
						m_order = new IdleOrder();
				}
				else {
					m_order = std::move(m_following_orders.front());
					m_following_orders.erase(m_following_orders.begin());
				}
			}

			handleOrder(EntityEvent::init_order);
			need_replicate = true;
		}
		
		if(need_replicate)
			replicate();
		
		handleOrder(EntityEvent::think);
		
		if(m_ai)
			m_ai->think();
	}

	void ThinkingEntity::nextFrame() {
		Entity::nextFrame();
		ThinkingEntity::handleOrder(EntityEvent::next_frame);	
	}

	void ThinkingEntity::onAnimFinished() {
		handleOrder(EntityEvent::anim_finished);
	}

	void ThinkingEntity::onPickupEvent() {
		handleOrder(EntityEvent::pickup);
	}
		
	void ThinkingEntity::onFireEvent(const int3 &off) {
		handleOrder(EntityEvent::fire, EntityEventParams{off, false});
	}
		
	void ThinkingEntity::onHitEvent() {
		handleOrder(EntityEvent::hit, EntityEventParams());
	}
		
	void ThinkingEntity::onStepEvent(bool left_foot) {
		handleOrder(EntityEvent::step, EntityEventParams{ int3(), left_foot });
	}

	void ThinkingEntity::onSoundEvent() {
		handleOrder(EntityEvent::sound);
	}
		
	void ThinkingEntity::onImpact(DamageType::Type dmg_type, float damage, const float3 &force, EntityRef source) {
		if(m_ai)
			m_ai->onImpact(dmg_type, damage, force, source);
	}
	
	void ThinkingEntity::detachAI() {
		m_ai.reset();
	}

	Brain *ThinkingEntity::AI() const {
		return m_ai.get();
	}

	const FBox ThinkingEntity::shootingBox(const Weapon &weapon) const {
		return boundingBox();
	}

	float ThinkingEntity::accuracy(const Weapon &weapon) const {
		return weapon.proto().accuracy;
	}
		
	float ThinkingEntity::inaccuracy(const Weapon &weapon) const {
		return 1.0f / max(10.0f, accuracy(weapon));
	}
	
	const Segment ThinkingEntity::computeBestShootingRay(const FBox &target_box, const Weapon &weapon) {
		//PROFILE_RARE("ThinkingEntity::shootingRay");

		FBox shooting_box = shootingBox(weapon);
		float3 center = shooting_box.center();
		float3 dir = normalized(target_box.center() - center);

		float3 source; {
			vector<float3> sources = genPointsOnPlane(shooting_box, dir, 5, true);
			vector<float3> targets = genPointsOnPlane(target_box, -dir, 5, false);

			int best_source = -1, best_hits = 0;
			float best_dist = 0.0f;

			vector<Segment> segments;
			for(int s = 0; s < (int)sources.size(); s++)
				for(int t = 0; t < (int)targets.size(); t++)
					segments.push_back(Segment(sources[s], targets[t]));

			vector<Intersection> results;
			world()->traceCoherent(segments, results, {Flags::all | Flags::colliding, ref()});

			int errors = 0, ok = 0;
			for(int s = 0; s < (int)sources.size(); s++) {
				int num_hits = 0;
		
				for(int t = 0; t < (int)targets.size(); t++) {
					const Segment &segment = segments[s * targets.size() + t];
					const Intersection &isect = results[s * targets.size() + t];

					if(isect.isEmpty() || isect.distance() + constant::epsilon >= intersection(segment, target_box))
						num_hits++;
				}

				float dist = distance(sources[s], center);
				if(best_source == -1 || num_hits > best_hits || (num_hits == best_hits && dist < best_dist)) {
					best_source = s;
					best_hits = num_hits;
					best_dist = dist;
				}
			}
			
#ifdef DEBUG_SHOOTING
			m_aiming_points.clear();
			m_aiming_points.insert(m_aiming_points.begin(), sources.begin(), sources.end());
#endif

			source = sources[best_source];
		}

		float3 best_target = target_box.center(); {
			float best_score = -constant::inf;
		
			vector<float3> targets = genPointsOnPlane(target_box, normalized(source - target_box.center()), 8, false);
			vector<char> target_hits(targets.size(), 0);

			vector<Segment> segments;
			for(int t = 0; t < (int)targets.size(); t++)
				segments.push_back(Segment(source, targets[t]));

			vector<Intersection> isects;
			world()->traceCoherent(segments, isects, {Flags::all | Flags::colliding, ref()});

			int num_hits = 0;
			for(int t = 0; t < (int)targets.size(); t++) {
				const Intersection &isect = isects[t];
				const Segment &segment = segments[t];
				if(isect.isEmpty() || isect.distance() + constant::epsilon >= intersection(segment, target_box)) {
					target_hits[t] = 1;
					num_hits++;
				}
			}

			for(int t = 0; t < (int)targets.size(); t++) {
				float score = 0.0f;
				float3 current = targets[t];
				if(!target_hits[t])
					continue;

#ifdef DEBUG_SHOOTING
				m_aiming_points.push_back(targets[t]);
#endif

				for(int i = 0; i < (int)targets.size(); i++)
					score += (target_hits[i]? 1.0f : 0.0f) / (1.0f + distanceSq(current, targets[i]));
				if(score > best_score) {
					best_score = score;
					best_target = current;
				}
			}
		}

		return Segment(source, best_target);
	}
	
	float ThinkingEntity::estimateHitChance(const Weapon &weapon, const FBox &target_bbox) {
	//	PROFILE_RARE("ThinkingEntity::estimateHitChance");

		Segment segment = computeBestShootingRay(target_bbox, weapon);

		float inaccuracy = this->inaccuracy(weapon);
		vector<Segment> segments;
		int density = 32;

		for(int x = 0; x < density; x++)
			for(int y = 0; y < density; y++) {
				float mul = 1.0f / (density - 1);
				Ray ray(segment.origin(), perturbVector(segment.dir(), float(x) * mul, float(y) *mul, inaccuracy));
				float dist = intersection(ray, target_bbox);
				if(dist < constant::inf)
					segments.push_back(Segment(ray, 0.0f, dist));
			}

		vector<Intersection> isects;
		world()->traceCoherent(segments, isects, {Flags::all | Flags::colliding, ref()});

		int num_hits = 0;
		for(int n = 0; n < (int)isects.size(); n++)
			if(isects[n].isEmpty() || isects[n].distance() + constant::epsilon >= intersection(segments[n], target_bbox))
			   num_hits++;	

		return float(num_hits) / (density * density);
	}

}
