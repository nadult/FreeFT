// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/base.h"

namespace game {

	// Order can be cancelled forcefully without any notification
	class Order {
	public:
		Order();
		Order(MemoryStream&);
		virtual ~Order() { }

		static Order *construct(MemoryStream&);
		virtual void save(MemoryStream&) const;
		
		void setFollowup(POrder &&followup) { m_followup = std::move(followup); }
		POrder &&getFollowup() { return std::move(m_followup); }
		bool hasFollowup() const { return (bool)m_followup; }
		
		virtual OrderTypeId typeId() const = 0;
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

	template <class TOrder,  OrderTypeId type_id_>
	class OrderImpl: public Order {
	public:
		static constexpr int type_id = (int)type_id_;

		OrderImpl() { }
		OrderImpl(MemoryStream &sr) :Order(sr) { }

		Order *clone() const { return new TOrder(*static_cast<const TOrder*>(this)); }
		OrderTypeId typeId() const { return type_id_; }
	};

}

#include "game/thinking_entity.h"

namespace game {

	template <class TOrder>
	bool ThinkingEntity::handleOrderWrapper(Order *order, EntityEvent event, const EntityEventParams &params) {
		DASSERT(order && order->typeId() == (OrderTypeId)TOrder::type_id);
		return handleOrder(*static_cast<TOrder*>(order), event, params);
	}

}
