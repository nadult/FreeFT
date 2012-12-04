#ifndef GFX_SPRITE_H
#define GFX_SPRITE_H

#include "gfx/texture.h"

namespace gfx {

	class Sprite: public RefCounter {
	public:
		void loadFromSpr(Serializer &sr);
		void serialize(Serializer &sr);

		struct Image {
			Image() :size(0, 0) { }
			void serialize(Serializer &sr);

			vector<u8> color;
			vector<u8> alpha;
			int2 size;
		};

		enum EventId {
			ev_stop_anim		= -2,
			ev_time_of_display	= -3,
			ev_repeat_all		= -4,
			ev_jump_to_frame	= -5,
			ev_overlay			= -6,

			ev_first_specific	= -40,	// these events wont be handled directly by Entity
			ev_step_left		= -40,
			ev_step_right		= -41,
			ev_hit				= -42,
			ev_fire 			= -43,
			ev_sound			= -44,
			ev_pickup			= -45,
		};

		struct Frame {
			Frame() :id(0) { params[0] = params[1] = params[2] = 0; }
			static int paramCount(char id);

			char id; // frame_id if >= 0, event_id if < 0
			char params[3];
		};

		struct Sequence {
			string name;
			vector<Frame> frames;
			int collection_id;
		};

		struct Collection {
			string name;
			vector<IRect> rects;
			vector<Image> images;
			vector<Color> palettes[4];
			vector<int2> points;

			Texture getFrame(int frameId, int dirId) const;

			int m_first_frame, m_frame_count, m_dir_count, m_offset;
			int type;
		};

		int dirCount(int seq_id) const { return m_collections[m_sequences[seq_id].collection_id].m_dir_count; }
		int frameCount(int seq_id) const;
		bool isSequenceLooped(int seq_id) const;

		Texture getFrame(int seq_id, int frameId, int dirId, IRect *rect = nullptr) const;
		int findSequence(const char *name) const;

		void printInfo() const;
		void printSequenceInfo(int seq_id) const;

		int findDir(int seq_id, float radians) const;

		int size() const { return (int)m_sequences.size(); }
		const Sequence &operator[](int seq_id) const { return m_sequences[seq_id]; }

		const int3 &boundingBox() const { return m_bbox; }
		int2 offset() const { return m_offset; }
		
		static ResourceMgr<Sprite> mgr;

	private:
		vector<Collection> m_collections;
		vector<Sequence> m_sequences;

		int2 m_offset;
		int3 m_bbox;
	};

	typedef Ptr<Sprite> PSprite;

};

#endif

