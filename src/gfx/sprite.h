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

		struct Sequence {
			string name;
			vector<int> frames;
			int anim_id;

		};

		struct Rect {
			int left, top;
			int right, bottom;
		};

		struct Animation {
			string name;
			vector<Rect> rects;
			vector<Image> images;
			vector<Color> palettes[4];
			vector<int2> points;

			Texture getFrame(int frameId, int dirId) const;

			int m_first_frame, m_frame_count, m_dir_count, m_offset;
			int type;
		};

		struct Frame {
			short x, y;
			short width, height;
		};
	
		int dirCount(int seq_id) const { return m_anims[m_sequences[seq_id].anim_id].m_dir_count; }
		int frameCount(int seq_id) const;

		Texture getFrame(int seq_id, int frameId, int dirId, Rect *rect = nullptr) const;
		int findSequence(const char *name) const;

		void printInfo() const;
		void printSequenceInfo(int seq_id) const;

		static int findDir(int dx, int dz);

		vector<Frame> m_frames;
		vector<Animation> m_anims;
		vector<Sequence> m_sequences;

		int2 m_offset;
		int3 m_bbox;
		
		static ResourceMgr<Sprite> mgr;
	};

	typedef Ptr<Sprite> PSprite;

};

#endif

