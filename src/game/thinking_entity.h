/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_THINKING_ENTITY_H
#define GAME_THINKING_ENTITY_H

#include "game/entity.h"

namespace game {

	class IdleOrder;
	class LookAtOrder;
	class MoveOrder;
	class TrackOrder;
	class AttackOrder;
	class ChangeStanceOrder;
	class InteractOrder;
	class DropItemOrder;
	class EquipItemOrder;
	class UnequipItemOrder;
	class TransferItemOrder;
	class GetHitOrder;
	class DieOrder;
	
	class Brain;
	using PBrain = ::ClonablePtr<Brain>;

	DEFINE_ENUM(EntityEvent,
		init_order,
		think,
		anim_finished,
		next_frame,
		sound,
		step,
		pickup,
		fire,
		hit,
		impact
	);

	struct EntityEventParams {
		int3 fire_offset;
		bool step_is_left;
	};

	// TODO: check for exception safety everywhere, where Entity* is used
	class ThinkingEntity: public Entity {
	public:
		ThinkingEntity(const Sprite &sprite);
		ThinkingEntity(const Sprite &sprite, const XMLNode&);
		ThinkingEntity(const Sprite &sprite, Stream&);
		ThinkingEntity(const ThinkingEntity&);
		~ThinkingEntity();
		
		XMLNode save(XMLNode& parent) const override;
		void save(Stream&) const override;
		
		template <class TAI, class ...Args>
		void attachAI(World *world, const Args&... args);
		void detachAI();
		Brain *AI() const;
		
		virtual bool setOrder(POrder&&, bool force = false);

		//TODO: move shooting functions to separate class?
		virtual const FBox shootingBox(const Weapon &weapon) const;
		virtual float accuracy(const Weapon &weapon) const;
		float inaccuracy(const Weapon &weapon) const;

		const Segment computeBestShootingRay(const FBox &bbox, const Weapon &weapon);
		float estimateHitChance(const Weapon &weapon, const FBox &bbox);
		
		Maybe<OrderTypeId> currentOrder() const;
		
		virtual bool canSee(EntityRef ref, bool simple_test = false) = 0;
		
		virtual bool isDying() const = 0;
		virtual bool isDead() const = 0;
		virtual int factionId() const { return -1; }

	protected:
		void think() override;
		void nextFrame() override;
	
		void onFireEvent(const int3 &projectile_offset) override;
		void onHitEvent() override;
		void onSoundEvent() override;
		void onStepEvent(bool left_foot) override;
		void onPickupEvent() override;

		void onAnimFinished() override;
		void onImpact(DamageType, float damage, const float3 &force, EntityRef source) override;

		void handleOrder(EntityEvent, const EntityEventParams &params = EntityEventParams{});
		bool failOrder() const;
		

		POrder m_order;
		vector<POrder> m_following_orders;
		PBrain m_ai;
		
		vector<float3> m_aiming_points;
		vector<float3> m_aiming_lines;

	private:
		typedef bool (ThinkingEntity::*HandleFunc)(Order*, EntityEvent, const EntityEventParams&);

		template <class TOrder>
		bool handleOrderWrapper(Order *order, EntityEvent event, const EntityEventParams &params);
		
		virtual bool handleOrder(IdleOrder&, EntityEvent, const EntityEventParams&) = 0;
		virtual bool handleOrder(LookAtOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(MoveOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(TrackOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(AttackOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(ChangeStanceOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(InteractOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(DropItemOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(EquipItemOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(UnequipItemOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(TransferItemOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(GetHitOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
		virtual bool handleOrder(DieOrder&, EntityEvent, const EntityEventParams&) { return failOrder(); }
	};

}

#endif

