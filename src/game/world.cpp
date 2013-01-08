#include "game/world.h"
#include "game/projectile.h"
#include "navigation_bitmap.h"
#include "sys/xml.h"
#include "sys/profiler.h"
#include "gfx/tile.h"
#include "gfx/scene_renderer.h"
#include "bvh_impl.h"
#include <cstdio>

namespace game {

	World::World()
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0), m_navi_map(2) { } 

	World::World(const char *file_name)
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0), m_navi_map(2) {
		XMLDocument doc;
		doc.load(file_name);

		TileMap tile_map;
		tile_map.loadFromXML(doc);
		m_tile_grid = tile_map;
		m_entity_grid = EntityGrid(tile_map.dimensions());
		m_tile_grid.printInfo();
	
		updateNavigationMap(true);
	}
	World::~World() {
	}

	void World::updateNavigationMap(bool full_recompute) {
		//PROFILE("updateNavigationMap");

		if(full_recompute) {
			NavigationBitmap bitmap(m_tile_grid, m_navi_map.extend());
			for(int n = 0; n < (int)m_entities.size(); n++)
				if(m_entities[n]->colliderType() == collider_static) {
					IBox box = enclosingIBox(m_entities[n]->boundingBox());
					bitmap.blit(IRect(box.min.xz(), box.max.xz()), false);
				}
			m_navi_map.update(bitmap);
			//m_navi_map.printInfo();
		}

		m_navi_map.removeColliders();
		for(int n = 0; n < (int)m_entities.size(); n++)
			if(m_entities[n]->colliderType() == collider_dynamic_nv) {
				const IBox &box = enclosingIBox(m_entities[n]->boundingBox());
				m_navi_map.addCollider(IRect(box.min.xz(), box.max.xz()));
			}

		//PROFILE("updateNavi::updateGrid");
		for(int n = 0; n < (int)m_entities.size(); n++) {
			const Entity *entity = m_entities[n].get();
			if(entity->colliderType() != collider_static)
				m_entity_grid.update(entity->m_grid_index,
						Grid::ObjectDef(entity, entity->boundingBox(), entity->screenRect(), entity->colliderType()));
		}
	}

	void World::addEntity(PEntity &&entity) {
		DASSERT(entity);
		entity->m_world = this;
		entity->m_grid_index =
			m_entity_grid.add(Grid::ObjectDef(entity.get(), entity->boundingBox(), entity->screenRect(), entity->colliderType()));
		m_entities.push_back(std::move(entity));
	}

	void World::addToRender(gfx::SceneRenderer &renderer) {
		PROFILE("World::addToRender");

		vector<int> tile_inds;
		tile_inds.reserve(1024);
		m_tile_grid.findAll(tile_inds, renderer.targetRect());
		for(int n = 0; n < (int)tile_inds.size(); n++) {
			const Grid::ObjectDef &obj = m_tile_grid[tile_inds[n]];
			((const gfx::Tile*)obj.ptr)->addToRender(renderer, (int3)obj.bbox.min);
		}

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
				if(object->m_grid_index != -1)
					m_entity_grid.remove(object->m_grid_index);

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
	
	Intersection World::trace(const Segment &segment, const Entity *ignore, ColliderFlags flags) const {
		PROFILE("world::trace");
		Intersection out;

		if(flags & collider_tiles) {
			pair<int, float> isect = m_tile_grid.trace(segment);
			if(isect.first != -1)
				out = Intersection(&m_tile_grid[isect.first], false, isect.second);
		}

		if(flags & collider_entities) {
			pair<int, float> isect = m_entity_grid.trace(segment, ignore? ignore->m_grid_index : -1, flags);
			if(isect.first != -1)
				out = Intersection(&m_entity_grid[isect.first], true, isect.second);
		}

		return out;
	}

	Intersection World::pixelIntersect(const int2 &screen_pos) const {
		//PROFILE("world::pixelIntersect");
		Intersection out;

		int tile_id = m_tile_grid.pixelIntersect(screen_pos, [](const Grid::ObjectDef &object, const int2 &pos)
				{ return ((const gfx::Tile*)object.ptr)->testPixel(pos - worldToScreen((int3)object.bbox.min)); } );
		if(tile_id != -1)
			out = Intersection(&m_tile_grid[tile_id], false, 0.0f);

		int entity_id = m_entity_grid.pixelIntersect(screen_pos, [](const Grid::ObjectDef &object, const int2 &pos)
				{ return ((const Entity*)object.ptr)->testPixel(pos); } );
		if(entity_id != -1) {
			const Grid::ObjectDef *object = &m_entity_grid[entity_id];
			if(tile_id == -1 || drawingOrder(object->bbox, out.boundingBox()) == 1)
				out = Intersection(object, true, 0.0f);
		}

		return out;
	}

	bool World::isColliding(const FBox &box, const Entity *ignore, ColliderFlags flags) const {
		//PROFILE("world::isColliding");

		if((flags & collider_tiles))
			if(m_tile_grid.findAny(box) != -1)
				return true;

		if(flags & collider_entities)
			if(m_entity_grid.findAny(box, ignore? ignore->m_grid_index : -1, flags) != -1)
				return true;

		return false;
	}

	bool World::isInside(const FBox &box) const {
		return m_tile_grid.isInside(box);
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
