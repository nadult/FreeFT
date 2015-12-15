/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef GAME_SPRITE_H
#define GAME_SPRITE_H

#include "game/base.h"
#include "gfx/packed_texture.h"
#include "gfx/texture_cache.h"

namespace game {

	//TODO: naming: toTexture, getFrame etc
	class Sprite: public immutable_base<Sprite> {
	public:
		Sprite();
		void legacyLoad(Stream &sr);
		void load(Stream &sr) { load(sr, true); }
		void load(Stream &sr, bool full_load);
		void save(Stream &sr) const;

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

		static const char *eventIdToString(EventId id);

		enum { layer_count = 4 };

		struct Frame {
			Frame() :id(0) { memset(params, 0, sizeof(params)); }
			static int paramCount(char id);

			int id;
			short params[4];
		};

		static_assert(sizeof(Frame) == 12, "Wrong size of Sprite::Frame");

		struct Sequence {
			void load(Stream&);
			void save(Stream&) const;

			string name;
			int frame_count, dir_count;
			int first_frame;
			int palette_id;
			int overlay_id;
		};

		struct MultiPalette {
			void load(Stream&);
			void save(Stream&) const;

			int size(int layer) const;
			const Color *access(int layer) const;
			bool operator==(const MultiPalette&) const;

			vector<Color> colors; //todo serialize as in Palette
			int offset[layer_count];
		};

		struct MultiImage: public CachedTexture {
			MultiImage();
			MultiImage(const MultiImage &rhs);
			void operator=(const MultiImage&);

			virtual void cacheUpload(Texture&) const;
			virtual int2 textureSize() const { return rect.size(); }

			void load(Stream&);
			void save(Stream&) const;

			STexture toTexture(const MultiPalette&, FRect&, bool put_in_atlas = true) const;
			bool testPixel(const int2&) const;
			int memorySize() const;

			PackedTexture images[layer_count];
			int2 points[layer_count];
			IRect rect;

			mutable const MultiPalette *prev_palette;
		};

		int dirCount(int seq_id) const { return m_sequences[seq_id].dir_count; }
		int frameCount(int seq_id) const { return m_sequences[seq_id].frame_count; }
		bool isSequenceLooped(int seq_id) const;

		int imageIndex(int seq_id, int frame_id, int dir_id) const;
		IRect getRect(int seq_id, int frame_id, int dir_id) const;
		IRect getMaxRect() const { return m_max_rect; }
		void updateMaxRect();
		bool testPixel(const int2 &screen_pos, int seq_id, int frame_id, int dir_id) const;
		
		STexture getFrame(int seq_id, int frame_id, int dir_id, FRect &tex_rect, bool put_in_atlas = true) const;
		
		// Search is case-insensitive
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

		int imageCount() const { return m_images.size(); }
		const MultiImage &image(int id) const { return m_images[id]; }
		
		int paletteCount() const { return m_palettes.size(); }
		const MultiPalette &palette(int id) const { return m_palettes[id]; }

		void clear();

		const int3 &bboxSize() const { return m_bbox; }

		static TextureCache cache;

		// Preloads basic information of all sprites
		// load sprites in full, when requested
		// TODO: add free/unload functions
		static void initMap();
		static int count();
		static int find(const string &name);
		static const Sprite &get(int idx);
		static const Sprite &get(const string &name);
		static const Sprite &getPartial(const string &name);
		static const Sprite &getDummy();
		static bool isValidIndex(int idx);

		bool isPartial() const { return m_is_partial; }
		int index() const { return m_index; }
		void setIndex(int index) { m_index = index; }
		const string &resourceName() const { return m_resource_name; }
		void setResourceName(const string &name) { m_resource_name = name; }

	private:
		vector<Sequence> m_sequences;
		vector<Frame> m_frames;
		vector<MultiPalette> m_palettes;
		vector<MultiImage> m_images;
		string m_resource_name;

		IRect m_max_rect;
		int2 m_offset;
		int3 m_bbox; //TODO: naming

		bool m_is_partial;
		int m_index;
	};

	using PSprite = shared_ptr<Sprite>;

};
	
SERIALIZE_AS_POD(game::Sprite::Frame);

#endif

