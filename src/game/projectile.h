/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "game/entity.h"

namespace game {

	class Projectile: public Entity {
	public:
		Projectile(ProjectileTypeId::Type type, float speed, const float3 &pos, float initial_ang,
					const float3 &target, Entity *spawner);
		Projectile(Stream&);
		
		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		virtual ColliderFlags colliderType() const { return collider_projectile; }
		virtual EntityId::Type entityType() const { return EntityId::projectile; }
		virtual Entity *clone() const;

		ProjectileTypeId::Type type() const { return m_type; }

	protected:
		virtual void think();
		void nextFrame();
		friend class World;

	private:
		ProjectileTypeId::Type m_type;
		EntityRef m_spawner;
		float3 m_dir;
		float m_speed, m_target_angle;
		int m_frame_count;
	};

	class Impact: public Entity {
	public:
		Impact(const char *sprite_name, const float3 &pos);
		Impact(Stream&);

		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		virtual ColliderFlags colliderType() const { return collider_projectile; }
		virtual EntityId::Type entityType() const { return EntityId::impact; }
		virtual Entity *clone() const;
	
	protected:
		virtual void onAnimFinished();
	};

}


#endif
