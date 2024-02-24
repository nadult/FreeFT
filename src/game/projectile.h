// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#pragma once

#include "game/entity.h"
#include "game/impact.h"
#include "sys/data_sheet.h"

namespace game {

struct ProjectileProto : public ProtoImpl<ProjectileProto, EntityProto, ProtoId::projectile> {
	ProjectileProto(const TupleParser &parser);
	void link();

	ProtoRef<ImpactProto> impact;
	float speed, max_range;
	bool blend_angles;
};

class Projectile : public EntityImpl<Projectile, ProjectileProto, EntityId::projectile> {
  public:
	Projectile(const ProjectileProto &Proto, float initial_ang, const float3 &dir,
			   EntityRef spawner, float damage_mod);
	Projectile(MemoryStream &);

	void addToRender(SceneRenderer &out, Color color) const override;

	void save(MemoryStream &) const override;
	XmlNode save(XmlNode parent) const override;

	Flags::Type flags() const override { return Flags::projectile | Flags::dynamic_entity; }

  protected:
	void think() override;
	void nextFrame() override;
	void onAnimFinished() override;
	void makeImpact(float3 new_pos, ObjectRef hit = ObjectRef());

  private:
	EntityRef m_spawner;
	float3 m_dir;
	float m_speed, m_distance, m_target_angle;
	float m_damage_mod;
	int m_frame_count;
	bool m_impact_created;
};

}
