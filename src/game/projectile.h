/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "game/entity.h"
#include "sys/data_sheet.h"

namespace game {
	
	struct ImpactDesc: public Tuple, TupleImpl<ImpactDesc> {
		ImpactDesc(const TupleParser &parser);

		string sprite_name;
	};

	struct ProjectileDesc: public Tuple, TupleImpl<ProjectileDesc> {
		ProjectileDesc(const TupleParser &parser);
		void connect();

		string sprite_name;
		TupleRef<ImpactDesc> impact_ref;
		float speed;
		bool blend_angles;
	};

	class Projectile: public Entity {
	public:
		Projectile(const ProjectileDesc &desc, const float3 &pos, float initial_ang,
					const float3 &target, Entity *spawner);
		Projectile(Stream&);
		
		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		virtual ColliderFlags colliderType() const { return collider_projectile; }
		virtual EntityId::Type entityType() const { return EntityId::projectile; }
		virtual Entity *clone() const;

	protected:
		virtual void think();
		void nextFrame();
		friend class World;

	private:
		const ProjectileDesc *m_desc;
		EntityRef m_spawner;
		float3 m_dir;
		float m_speed, m_target_angle;
		int m_frame_count;
	};

	class Impact: public Entity {
	public:
		Impact(const ImpactDesc&, const float3 &pos);
		Impact(Stream&);

		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		virtual ColliderFlags colliderType() const { return collider_projectile; }
		virtual EntityId::Type entityType() const { return EntityId::impact; }
		virtual Entity *clone() const;
	
	protected:
		virtual void onAnimFinished();
		const ImpactDesc *m_desc;
	};

}


#endif
