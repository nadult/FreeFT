#ifndef GFX_SPRITE_H
#define GFX_SPRITE_H

#include "gfx/texture.h"

namespace gfx {

	class Sprite: public RefCounter {
	public:
		Sprite();
		void loadFromSpr(Serializer &sr);
		void serialize(Serializer &sr);

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
			vector<CompressedTexture> images;
			vector<Color> palettes[4];
			vector<int2> points;

			void getLayerIndices(int frame_id, int dir_id, int *layer_indices) const;
			Texture getFrame(int frame_id, int dir_id) const;
			bool testPixel(const int2 &screen_pos, int frame_id, int dir_id) const;

			int m_first_frame, m_frame_count, m_dir_count, m_offset;
			int type;
		};

		int dirCount(int seq_id) const { return m_collections[m_sequences[seq_id].collection_id].m_dir_count; }
		int frameCount(int seq_id) const;
		bool isSequenceLooped(int seq_id) const;

		int accessFrame(int seq_id, int frame_id, int dir_id) const;
		Texture getFrame(int seq_id, int frame_id, int dir_id) const;
		IRect getRect(int seq_id, int frame_id, int dir_id) const;
		IRect getMaxRect() const;
		bool testPixel(const int2 &screen_pos, int seq_id, int frame_id, int dir_id) const;
		
		int findSequence(const char *name) const;

		void printInfo() const;
		void printSequencesInfo() const;
		void printSequenceInfo(int seq_id) const;

		int findDir(int seq_id, float radians) const;

		int size() const { return (int)m_sequences.size(); }
		const Sequence &operator[](int seq_id) const { return m_sequences[seq_id]; }

		const int3 &boundingBox() const { return m_bbox; }
		
		static ResourceMgr<Sprite> mgr;

	private:
		vector<Collection> m_collections;
		vector<Sequence> m_sequences;
		string m_name;
		int m_file_size;

		int2 m_offset;
		int3 m_bbox;
	};

	typedef Ptr<Sprite> PSprite;

};

#endif

