#include "game/world.h"
#include "navigation_bitmap.h"

namespace game {

	World::World()
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0) { } 

	World::World(const char *file_name)
		:m_last_frame_time(0.0), m_last_time(0.0), m_time_delta(0.0), m_current_time(0.0) {
		XMLDocument doc;
		loadXMLDocument(file_name, doc);
		m_tile_map.loadFromXML(doc);
	
		m_navi_map.update(NavigationBitmap(m_tile_map, 2));
		m_navi_map.printInfo();
	}

	void World::updateNavigationMap() {
		NavigationBitmap bitmap(m_tile_map, 2);
		for(int n = 0; n < (int)m_entities.size(); n++)
			if(m_entities[n]->isStatic()) {
				IBox box = m_entities[n]->boundingBox();
				bitmap.blit(IRect(box.min.xz(), box.max.xz()), false);
			}
		m_navi_map.update(bitmap);
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

		for(int n = 0; n < (int)m_entities.size(); n++) {
			Entity *entity = m_entities[n].get();
			entity->think();
			if(frame_skip)
				entity->animate(frame_skip);
			DASSERT(!isColliding(entity->boundingBox(), entity));
		}

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
		
	vector<int2> World::findPath(int2 start, int2 end) const {
		return m_navi_map.findPath(start, end);
	}

}
