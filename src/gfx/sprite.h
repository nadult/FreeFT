#ifndef GFX_SPRITE_H
#define GFX_SPRITE_H

#include "gfx/texture.h"
#include "gfx/device.h"
#include "gfx/texture_cache.h"

namespace gfx {

	class Sprite: public RefCounter {
	public:
		Sprite();
		void legacyLoad(Serializer &sr);
		void serialize(Serializer &sr);

		enum EventId {
			ev_stop_anim		= -2,
			ev_time_of_display	= -3,
			ev_repeat_all		= -4,
			ev_jump_to_frame	= -5,
			ev_overlay			= -6,

			ev_first_specific	= -40,	// these events won't be handled directly by Entity
			ev_step_left		= -40,
			ev_step_right		= -41,
			ev_hit				= -42,
			ev_fire 			= -43,
			ev_sound			= -44,
			ev_pickup			= -45,
		};

		enum { layer_count = 4 };

		struct Frame {
			Frame() :id(0) { memset(params, 0, sizeof(params)); }
			static int paramCount(char id);

			int id;
			short params[4];
		};

		static_assert(sizeof(Frame) == 12, "Wrong size of Sprite::Frame");

		struct Sequence {
			void serialize(Serializer&);

			string name;
			int frame_count, dir_count;
			int first_frame;
			int palette_id;
		};

		struct MultiPalette {
			void serialize(Serializer&);
			int size(int layer) const;
			const Color *access(int layer) const;
			bool operator==(const MultiPalette&) const;

			vector<Color> colors;
			int offset[layer_count];
		};

		struct MultiImage {
			MultiImage();
			~MultiImage();
			MultiImage(const MultiImage &rhs);
			void operator=(const MultiImage&);

			void serialize(Serializer&);
			PTexture toTexture(const MultiPalette&) const;
			bool testPixel(const int2&) const;
			int memorySize() const;

			PackedTexture images[layer_count];
			int2 points[layer_count];
			IRect rect;

			mutable const MultiPalette *prev_palette;
			mutable int cache_id;
		};

		int dirCount(int seq_id) const { return m_sequences[seq_id].dir_count; }
		int frameCount(int seq_id) const { return m_sequences[seq_id].frame_count; }
		bool isSequenceLooped(int seq_id) const;

		int imageIndex(int seq_id, int frame_id, int dir_id) const;
		PTexture getFrame(int seq_id, int frame_id, int dir_id) const;
		IRect getRect(int seq_id, int frame_id, int dir_id) const;
		IRect getMaxRect() const;
		bool testPixel(const int2 &screen_pos, int seq_id, int frame_id, int dir_id) const;
		
		int findSequence(const char *name) const;

		int memorySize() const;
		void printInfo() const;
		void printSequencesInfo() const;
		void printSequenceInfo(int seq_id) const;

		int findDir(int seq_id, float radians) const;

		int size() const { return (int)m_sequences.size(); }
		const Sequence &operator[](int seq_id) const { return m_sequences[seq_id]; }
		
		const Frame &frame(int seq_id, int frame_id) const
			{ return m_frames[m_sequences[seq_id].first_frame + frame_id]; }

		void clear();

		const int3 &boundingBox() const { return m_bbox; }
		
		static ResourceMgr<Sprite> mgr;
		static TextureCache cache;

	private:
		vector<Sequence> m_sequences;
		vector<Frame> m_frames;
		vector<MultiPalette> m_palettes;
		vector<MultiImage> m_images;

		string m_name;
		int2 m_offset;
		int3 m_bbox; //TODO: naming
	};

	typedef Ptr<Sprite> PSprite;

};
	
SERIALIZE_AS_POD(gfx::Sprite::Frame);

#endif

