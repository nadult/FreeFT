/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "game/entity.h"
#include "sys/data_sheet.h"

namespace game {
	
	struct ImpactProto: public ProtoImpl<ImpactProto, EntityProto, ProtoId::impact> {
		ImpactProto(const TupleParser &parser) :ProtoImpl(parser) { }
	};

	struct ProjectileProto: public ProtoImpl<ProjectileProto, EntityProto, ProtoId::projectile> {
		ProjectileProto(const TupleParser &parser);
		void connect();

		ProtoRef<ImpactProto> impact;
		float speed;
		bool blend_angles;
	};

	class Projectile: public EntityImpl<Projectile, ProjectileProto, EntityId::projectile> {
	public:
		Projectile(const ProjectileProto &Proto, const float3 &pos, float initial_ang,
					const float3 &target, Entity *spawner);
		Projectile(Stream&);
		
		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		virtual ColliderFlags colliderType() const { return collider_projectile; }

	protected:
		virtual void think();
		void nextFrame();
		friend class World;

	private:
		EntityRef m_spawner;
		float3 m_dir;
		float m_speed, m_target_angle;
		int m_frame_count;
	};

	class Impact: public EntityImpl<Impact, ImpactProto, EntityId::impact> {
	public:
		Impact(const ImpactProto&, const float3 &pos);
		Impact(Stream&);

		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		virtual ColliderFlags colliderType() const { return collider_projectile; }
	
	protected:
		virtual void onAnimFinished();
	};

}


#endif
