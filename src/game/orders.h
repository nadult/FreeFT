/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_ORDERS_H
#define GAME_ORDERS_H

#include "game/base.h"

namespace game {

/*	class POrder: public ClonablePtr<Order> {
	public:
		using ClonablePtr<Order>::ClonablePtr;

		void save(Stream&) const;
		void load(Stream&);
	};*/

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

#include "game/thinking_entity.h"

namespace game {

	template <class TOrder>
	bool ThinkingEntity::handleOrderWrapper(Order *order, EntityEvent::Type event, const EntityEventParams &params) {
		DASSERT(order && order->typeId() == (OrderTypeId::Type)TOrder::type_id);
		return handleOrder(*static_cast<TOrder*>(order), event, params);
	}


}

#endif

