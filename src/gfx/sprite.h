#include "gfx/texture.h"

namespace gfx {

	class Sprite {
	public:
		void LoadFromSpr(Serializer &sr);

		struct Image {
			Image() :size(0, 0) { }
			void Serialize(Serializer &sr);

			vector<u8> color;
			vector<u8> alpha;
			int2 size;
		};

		struct Sequence {
			string name;
			vector<int> frames;
			int animId;

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

			Texture GetFrame(int frameId, int dirId) const;

			int firstFrame, numFrames, numDirs, offset;
			int type;
		};

		struct Frame {
			short x, y;
			short width, height;
		};
	
		int NumDirs(int seqId) const { return anims[sequences[seqId].animId].numDirs; }
		int NumFrames(int seqId) const;
		Texture GetFrame(int seqId, int frameId, int dirId) const;

		Texture texture;
		vector<Frame> frames;
		vector<Animation> anims;
		vector<Sequence> sequences;
	};

};
