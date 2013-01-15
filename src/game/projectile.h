/* Copyright (C) 2013 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FTremake.

   FTremake is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   FTremake is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef GAME_PROJECTILE_H
#define GAME_PROJECTILE_H

#include "game/entity.h"

namespace game {

   	//TODO: derived from Entity mainly for sprite animation handling,
	//maybe this class should be more lightweight
	class Projectile: public Entity {
	public:
		Projectile(ProjectileTypeId::Type type, float speed, const float3 &pos,
					const float3 &target, Entity *spawner);
		virtual ColliderFlags colliderType() const { return collider_none; }
		virtual EntityId::Type entityType() const { return EntityId::projectile; }

		ProjectileTypeId::Type type() const { return m_type; }

	protected:
		virtual void think();
		friend class World;

	private:
		ProjectileTypeId::Type m_type;
		EntityRef m_spawner;
		float3 m_dir;
		float m_speed;
	};

	class ProjectileImpact: public Entity {
	public:
		ProjectileImpact(const char *sprite_name, const float3 &pos);
		virtual ColliderFlags colliderType() const { return collider_none; }
		virtual EntityId::Type entityType() const { return EntityId::impact; }
	
	protected:
		virtual void onAnimFinished();
	};

	typedef std::unique_ptr<Projectile> PProjectile;
	typedef std::unique_ptr<ProjectileImpact> PProjectileImpact;

}


#endif
