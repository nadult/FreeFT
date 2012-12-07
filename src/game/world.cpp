#include "game/world.h"
#include "game/projectile.h"
#include "navigation_bitmap.h"

namespace game {

	World::World()
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0), m_navi_map(2) { } 

	World::World(const char *file_name)
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0), m_navi_map(2) {
		XMLDocument doc;
		loadXMLDocument(file_name, doc);
		m_tile_map.loadFromXML(doc);
	
		updateNavigationMap(true);
	}

	void World::updateNavigationMap(bool full_recompute) {
		if(full_recompute) {
			NavigationBitmap bitmap(m_tile_map, m_navi_map.extend());
			for(int n = 0; n < (int)m_entities.size(); n++)
				if(m_entities[n]->colliderType() == collider_static) {
					IBox box = m_entities[n]->boundingBox();
					bitmap.blit(IRect(box.min.xz(), box.max.xz()), false);
				}
			m_navi_map.update(bitmap);
			m_navi_map.printInfo();
		}

		m_navi_map.removeColliders();
		for(int n = 0; n < (int)m_entities.size(); n++)
			if(m_entities[n]->colliderType() == collider_dynamic) {
				const IBox &box = m_entities[n]->boundingBox();
				m_navi_map.addCollider(IRect(box.min.xz(), box.max.xz()));
			}
	}

	void World::addEntity(PEntity &&entity) {
		DASSERT(entity);
		entity->m_world = this;
		m_entities.push_back(std::move(entity));
	}

	void World::addToRender(gfx::SceneRenderer &renderer) {
		m_tile_map.addToRender(renderer);
		for(int n = 0; n < (int)m_entities.size(); n++)
			m_entities[n]->addToRender(renderer);
		for(int n = 0; n < (int)m_projectiles.size(); n++)
			m_projectiles[n]->addToRender(renderer);
		for(int n = 0; n < (int)m_impacts.size(); n++)
			m_impacts[n]->addToRender(renderer);
	}

	template <class T>
	void World::handleContainer(vector<std::unique_ptr<T> > &objects, int frame_skip) {
		for(int n = 0; n < (int)objects.size(); n++) {
			T *object = objects[n].get();
			object->think();
			if(object->m_to_be_removed) {
				objects[n--] = std::move(objects.back());
				objects.pop_back();
				continue;
			}

			for(int f = 0; f < frame_skip; f++)
				object->nextFrame();
			//DASSERT(!isColliding(object->boundingBox(), object));
		}
	}

	void World::simulate(double time_diff) {
		DASSERT(time_diff >= 0.0);
		double max_time_diff = 1.0; //TODO: add warning?
		time_diff = min(time_diff, max_time_diff);

		double current_time = m_last_time + time_diff; //TODO: rozjedzie sie z getTime(), ale czy to jest problem?
		double frame_diff = current_time - m_last_frame_time;
		double frame_time = 1.0 / 15.0, fps = 15.0;
		m_current_time = current_time;
		m_time_delta = time_diff;

		int frame_skip = 0;
		if(frame_diff > frame_time) {
			frame_skip = (int)(frame_diff * fps);
			m_last_frame_time += (double)frame_skip * frame_time;
		}

		handleContainer(m_entities, frame_skip);
		handleContainer(m_projectiles, frame_skip);
		handleContainer(m_impacts, frame_skip);

		m_last_time = current_time;
	}
	
	Intersection World::intersectEntities(const Ray &ray, float tmin, float tmax) const {
		Intersection out;

		for(int n = 0; n < (int)m_entities.size(); n++) {
			IBox box = m_entities[n]->boundingBox();
			float dist = intersection(ray, (Box<float3>)box);
			if(dist < out.t && dist >= tmin && dist <= tmax)
				out = Intersection(m_entities[n].get(), dist);
		}

		return out;
	}

	bool World::isColliding(const IBox &box, const Entity *ignore) const {
		if(m_tile_map.isOverlapping(box))
			return true;

		for(int n = 0; n < (int)m_entities.size(); n++) {
			const Entity *entity = m_entities[n].get();
			if(entity != ignore && areOverlapping(entity->boundingBox(), box))
				return true;
		}

		return false;
	}

	bool World::isInside(const IBox &box) const {
		return box.min.x >= 0 && box.min.y >= 0 && box.min.z >= 0 &&
				box.max.x < m_tile_map.size().x && box.max.z < m_tile_map.size().y &&
				box.max.y < TileMapNode::size_y;
	}
		
	vector<int2> World::findPath(int2 start, int2 end) const {
		return m_navi_map.findPath(start, end);
	}
	
	void World::spawnProjectile(int type, const int3 &pos, const int3 &target, Entity *spawner) {
		std::unique_ptr<Projectile> projectile(new Projectile("impactfx/Projectile Plasma", pos, target, spawner));
		projectile->m_world = this;
		m_projectiles.push_back(std::move(projectile));
	}
	
	void World::spawnProjectileImpact(int type, const int3 &pos) {
		std::unique_ptr<ProjectileImpact> impact(new ProjectileImpact("impactfx/Impact Plasma", pos));
		impact->m_world = this;
		m_impacts.push_back(std::move(impact));
	}

}
