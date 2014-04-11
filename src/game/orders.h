/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_H
#define GAME_ORDERS_H

#include "game/base.h"

namespace game {

	// Only actors can receive orders

	DECLARE_ENUM(OrderTypeId,
		invalid = -1,
	
		idle = 0,
		look_at,
		move,
		track,
		attack,
		change_stance,
		interact,
		drop_item,
		equip_item,
		unequip_item,
		transfer_item,
		get_hit,
		die
	);

	namespace ActorEvent {
		enum Type {
			init_order		= 0x001,
			think			= 0x002,
			anim_finished	= 0x004,
			next_frame		= 0x008,
			sound			= 0x010,
			step			= 0x020,
			pickup			= 0x040,
			fire			= 0x080,
			hit				= 0x100,
			impact			= 0x200,
		};
	};

	struct ActorEventParams {
		int3 fire_offset;
		bool step_is_left;
	};

	class POrder: public ClonablePtr<Order> {
	public:
		using ClonablePtr<Order>::ClonablePtr;

		void save(Stream&) const;
		void load(Stream&);
	};

	// Order can be cancelled forcefully without any notification
	class Order {
	public:
		Order();
		Order(Stream&);
		virtual ~Order() { }

		static Order *construct(Stream&);
		virtual void save(Stream&) const;
		
		void setFollowup(POrder &&followup) { m_followup = std::move(followup); }
		POrder &&getFollowup() { return std::move(m_followup); }
		bool hasFollowup() const { return (bool)m_followup; }
		
		virtual OrderTypeId::Type typeId() const = 0;
		virtual Order *clone() const = 0;

		bool isFinished() const { return m_is_finished; }
		bool needCancel() const { return m_please_cancel; }

		void finish() { m_is_finished = true; }
		virtual void cancel() { m_please_cancel = true; }
		
	private:
		POrder m_followup;
		bool m_is_finished;
		bool m_please_cancel;
	};

	template <class TOrder,  OrderTypeId::Type type_id_>
	class OrderImpl: public Order {
	public:
		enum {
			type_id = type_id_,
		};

		OrderImpl() { }
		OrderImpl(Stream &sr) :Order(sr) { }

		Order *clone() const { return new TOrder(*static_cast<const TOrder*>(this)); }
		OrderTypeId::Type typeId() const { return type_id_; }
	};

}

#endif

#include "game/orders/idle.h"
#include "game/orders/look_at.h"
#include "game/orders/move.h"
#include "game/orders/track.h"
#include "game/orders/attack.h"
#include "game/orders/change_stance.h"
#include "game/orders/interact.h"
#include "game/orders/inventory.h"
#include "game/orders/die.h"
#include "game/orders/get_hit.h"


