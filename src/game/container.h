/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_CONTAINER_H
#define GAME_CONTAINER_H

#include "game/entity.h"
#include "game/inventory.h"

namespace game
{

	class Container: public Entity
	{
		void initialize(const char *sprite_name, const float3 &pos);

	public:
		enum State {
			state_closed,
			state_opened,
			state_opening,
			state_closing,

			state_count
		};

		Container(Stream&);
		Container(const char *sprite_name, const float3 &pos) { initialize(sprite_name, pos); }

		virtual ColliderFlags colliderType() const { return collider_static; }
		virtual EntityId::Type entityType() const { return EntityId::container; }
		virtual Entity *clone() const;

		void open();
		void close();
		void setKey(const Item&);

		virtual void interact(const Entity*);

		bool isOpened() const { return m_state == state_opened; }
		bool isAlwaysOpened() const { return m_is_always_opened; }

		const Inventory &inventory() const { return m_inventory; }
		Inventory &inventory() { return m_inventory; }
		
	private:
		virtual void saveContentsToXML(XMLNode&) const;
		virtual void saveToBinary(Stream&);

		virtual void think();
		virtual void onAnimFinished();

		State m_state, m_target_state;
		bool m_is_always_opened;
		bool m_update_anim;
		Item m_key;

		int m_seq_ids[state_count];
		Inventory m_inventory;
	};
};

#endif
