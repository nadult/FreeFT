#ifndef GAME_ENTITY_H
#define GAME_ENTITY_H

#include "base.h"
#include <memory>

namespace gfx
{
	class SceneRenderer;
	class Sprite;
	typedef Ptr<Sprite> PSprite;
}

namespace game {

	using gfx::Sprite;
	class World;

	//TODO: static polimorphism where its possible, or maybe even
	// array for each type of Entity
	class Entity {
	public:
		Entity(const char *sprite_name, const int3 &pos);
		virtual ~Entity();

		virtual void addToRender(gfx::SceneRenderer&) const;

		void roundPos();
		void setPos(float3);
		float3 pos() const { return m_pos; }
		IBox boundingBox() const;
		const int3 &bboxSize() const { return m_bbox; }

		int dir() const { return m_dir; }

	protected:
		friend class World;
		const World *m_world;

	protected:
		virtual void think() { DASSERT(m_world); }
		virtual void animate(int frame_skip);

		void playSequence(int seq_id);

		void setDir(int new_dir);
		
		virtual void onAnimFinished() { }

		gfx::PSprite m_sprite;

	private:
		float3 m_pos; //TODO: int3 pos + float3 offset
		int3 m_bbox;

		struct AnimState {
			int seq_id, frame_id;
			bool is_looped;
		} m_anim_state;
		int m_dir;
	};

	typedef std::unique_ptr<Entity> PEntity;

}

#endif
