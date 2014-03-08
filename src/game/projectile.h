/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "game/entity.h"
#include "sys/data_sheet.h"

namespace game {
	
	struct ImpactProto: public ProtoImpl<ImpactProto, EntityProto, ProtoId::impact> {
		ImpactProto(const TupleParser &parser);

		SoundId sound_idx;
	};

	struct ProjectileProto: public ProtoImpl<ProjectileProto, EntityProto, ProtoId::projectile> {
		ProjectileProto(const TupleParser &parser);
		void connect();

		ProtoRef<ImpactProto> impact;
		DeathTypeId::Type death_id;
		float speed;
		bool blend_angles;
	};

	class Projectile: public EntityImpl<Projectile, ProjectileProto, EntityId::projectile> {
	public:
		Projectile(const ProjectileProto &Proto, float initial_ang, const float3 &dir, EntityRef spawner);
		Projectile(Stream&);
		
		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		ColliderFlags colliderType() const { return collider_projectile; }

	protected:
		virtual void think();
		void nextFrame();

	private:
		EntityRef m_spawner;
		float3 m_dir;
		float m_speed, m_target_angle;
		int m_frame_count;
	};

	class Impact: public EntityImpl<Impact, ImpactProto, EntityId::impact> {
	public:
		Impact(const ImpactProto&);
		Impact(Stream&);

		void save(Stream&) const;
		XMLNode save(XMLNode& parent) const;

		ColliderFlags colliderType() const { return collider_projectile; }
	
	protected:
		void onAnimFinished() override;
		void think() override;

		//TODO: Ugly hack; Add possibility to create entity with world as a parameter,
		//this way we will be able to play sounds that should be played on the first frame
		bool m_played_sound;
	};

}


#endif
