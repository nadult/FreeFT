#include "game/world.h"
#include "game/projectile.h"
#include "navigation_bitmap.h"
#include "sys/xml.h"
#include "sys/profiler.h"
#include <cstdio>

namespace game {

	WorldElement::WorldElement() :m_entity(nullptr), m_tile_node_id(-1) { }
	WorldElement::WorldElement(Entity *entity) :m_entity(entity), m_tile_node_id(-1) { }
	WorldElement::WorldElement(const TileMap *tile_map, int tile_node_id, int tile_instance_id)
		:m_tile_node_id(tile_node_id), m_tile_instance_id(tile_instance_id), m_tile_map(tile_map) {
			DASSERT(tile_map && tile_node_id != -1 && tile_instance_id != -1);
		}

	const FBox WorldElement::boundingBox() const {
		if(isEntity())
			return m_entity->boundingBox();
		if(isTile()) {
			const TileInstance &inst = (*m_tile_map)(m_tile_node_id)(m_tile_instance_id);
			return (FBox)(inst.boundingBox() + m_tile_map->nodePos(m_tile_node_id));
		}

		return FBox(0, 0, 0, 0, 0, 0);
	}

	World::World()
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0), m_navi_map(2) { } 

	World::World(const char *file_name)
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0), m_navi_map(2) {
		XMLDocument doc;
		doc.load(file_name);
		m_tile_map.loadFromXML(doc);
	
		updateNavigationMap(true);
	}

	void World::updateNavigationMap(bool full_recompute) {
		PROFILE("updateNavigationMap");

		if(full_recompute) {
			NavigationBitmap bitmap(m_tile_map, m_navi_map.extend());
			for(int n = 0; n < (int)m_entities.size(); n++)
				if(m_entities[n]->colliderType() == collider_static) {
					IBox box = enclosingIBox(m_entities[n]->boundingBox());
					bitmap.blit(IRect(box.min.xz(), box.max.xz()), false);
				}
			m_navi_map.update(bitmap);
		//	m_navi_map.printInfo();
		}

		m_navi_map.removeColliders();
		for(int n = 0; n < (int)m_entities.size(); n++)
			if(m_entities[n]->colliderType() == collider_dynamic_nv) {
				const IBox &box = enclosingIBox(m_entities[n]->boundingBox());
				m_navi_map.addCollider(IRect(box.min.xz(), box.max.xz()));
			}
	}

	void World::addEntity(PEntity &&entity) {
		DASSERT(entity);
		entity->m_world = this;
		m_entities.push_back(std::move(entity));
	}

	void World::addToRender(gfx::SceneRenderer &renderer) {
		PROFILE("World::addToRender");

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
		PROFILE("World::simulate");

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
	
	Intersection World::intersect(const Segment &segment, const Entity *ignore, ColliderFlags flags) const {
		PROFILE("world::intersect");
		Intersection out;

		if(flags & collider_tiles) {
			TileMap::Intersection isect = m_tile_map.intersect(segment);
			if(isect.node_id != -1)
				out = Intersection(WorldElement(&m_tile_map, isect.node_id, isect.instance_id), isect.distance);
		}

		if(flags & collider_entities)
			for(int n = 0; n < (int)m_entities.size(); n++) {
				Entity *entity = m_entities[n].get();
				if(!(entity->colliderType() & flags) || entity == ignore)
					continue;

				float dist = intersection(segment, entity->boundingBox());
				if(dist < out.distance && dist >= segment.min && dist <= segment.max)
					out = Intersection(entity, dist);
			}

		return out;
	}

	Intersection World::pixelIntersect(const int2 &screen_pos) const {
		PROFILE("world::pixelIntersect");
		FBox best_box;
		Intersection out;
		pair<int, int> tile = m_tile_map.pixelIntersect(screen_pos);
		if(tile.first != -1) {
			out = Intersection(WorldElement(&m_tile_map, tile.first, tile.second), 0.0f);
			best_box = FBox(m_tile_map(tile.first)(tile.second).boundingBox() + m_tile_map.nodePos(tile.first));
		}

		for(int n = 0; n < (int)m_entities.size(); n++) {
			Entity *entity = m_entities[n].get();
			if(!entity->pixelTest(screen_pos))
				continue;
			FBox box = entity->boundingBox();

			if(out.isEmpty() || drawingOrder(box, best_box) == 1) {
				out = Intersection(entity, 0.0f);
				best_box = box;
			}
		}

		return out;
	}

	bool World::isColliding(const FBox &box, const Entity *ignore, ColliderFlags flags) const {
		if((flags & collider_tiles) && m_tile_map.isOverlapping(enclosingIBox(box)))
			return true;

		if(flags & collider_entities)
			for(int n = 0; n < (int)m_entities.size(); n++) {
				const Entity *entity = m_entities[n].get();
				if(	(entity->colliderType() & flags) && entity != ignore) {
					FBox isect = intersection(entity->boundingBox(), box);
					if(isect.width() > constant::epsilon && isect.height() > constant::epsilon &&
						isect.depth() > constant::epsilon) {
						printf("collision: %f %f %f\n", isect.width(), isect.height(), isect.depth());
						return true;
					}
				}
			}

		return false;
	}

	bool World::isInside(const FBox &box) const {
		return box.min.x >= 0.0f && box.min.y >= 0.0f && box.min.z >= 0.0f &&
				box.max.x <= (float)m_tile_map.size().x && box.max.z <= (float)m_tile_map.size().y &&
				box.max.y <= (float)TileMapNode::size_y;
	}
		
	vector<int2> World::findPath(int2 start, int2 end) const {
		PROFILE_RARE("world::findPath");
		return m_navi_map.findPath(start, end);
	}
	
	void World::spawnProjectile(PProjectile projectile) {
		projectile->m_world = this;
		m_projectiles.push_back(std::move(projectile));
	}
	
	void World::spawnProjectileImpact(PProjectileImpact impact) {
		impact->m_world = this;
		m_impacts.push_back(std::move(impact));
	}

}
