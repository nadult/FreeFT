/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "game/sprite.h"
#include "sys/platform.h"
#include <cstring>
#include <cstdio>

using namespace gfx;

namespace game
{
			
	int Sprite::MultiPalette::size(int layer) const {
		int next = layer == layer_count - 1? (int)colors.size() : offset[layer + 1];
		return next - offset[layer];
	}

	const Color *Sprite::MultiPalette::access(int layer) const {
		return colors.data() + offset[layer];
	}

	bool Sprite::MultiPalette::operator==(const Sprite::MultiPalette &rhs) const {
		return colors == rhs.colors && memcmp(offset, rhs.offset, sizeof(offset)) == 0;
	}

	int Sprite::Frame::paramCount(char id) {
		if(id == ev_stop_anim || id == ev_jump_to_frame || id == ev_time_of_display)
			return 1;
		if(id == ev_fire)
			return 3;
		return 0;
	}

	Sprite::Sprite() :m_bbox(0, 0, 0), m_offset(0, 0) { }

	void Sprite::Sequence::load(Stream &sr) {
		sr >> name;
		sr.unpack(frame_count, dir_count, first_frame, palette_id);
	}

	void Sprite::Sequence::save(Stream &sr) const {
		sr << name;
		sr.pack(frame_count, dir_count, first_frame, palette_id);
	}

	void Sprite::MultiPalette::load(Stream &sr) {
		sr >> colors;
		sr.load(offset, sizeof(offset));
	}

	void Sprite::MultiPalette::save(Stream &sr) const {
		sr << colors;
		sr.save(offset, sizeof(offset));
	}

	void Sprite::MultiImage::load(Stream &sr) {
		for(int l = 0; l < 4; l++)
			sr >> images[l];
		sr.load(points, sizeof(points));
		sr >> rect;
	}

	void Sprite::MultiImage::save(Stream &sr) const {
		for(int l = 0; l < 4; l++)
			sr << images[l];
		sr.save(points, sizeof(points));
		sr << rect;
	}

	Sprite::MultiImage::MultiImage() :prev_palette(nullptr) { }
	
	Sprite::MultiImage::MultiImage(const MultiImage &rhs) :prev_palette(nullptr) {
		*this = rhs;
	}

	void Sprite::MultiImage::operator=(const MultiImage &rhs) {
		for(int n = 0; n < layer_count; n++) {
			images[n] = rhs.images[n];
			points[n] = rhs.points[n];
		}
		rect = rhs.rect;
	}

	int Sprite::MultiImage::memorySize() const {
		int bytes = sizeof(MultiImage) - sizeof(images);
		for(int n = 0; n < layer_count; n++)
			bytes += images[n].memorySize();
		return bytes;
	}

	void Sprite::MultiImage::cacheUpload(Texture &tex) const {
		DASSERT(prev_palette);

		tex.resize(rect.width(), rect.height());
		if(!tex.isEmpty()) {
			memset(tex.line(0), 0, rect.width() * rect.height() * sizeof(Color));
			for(int l = 0; l < Sprite::layer_count; l++)
				images[l].blit(tex, points[l], prev_palette->access(l), prev_palette->size(l));
		}
	}

	PTexture Sprite::MultiImage::toTexture(const MultiPalette &palette, FRect &tex_rect) const {
		if(cacheId() == -1) {
			bindToCache(TextureCache::main_cache);
			prev_palette = &palette;
		}

		return accessTexture(tex_rect);
	}

	bool Sprite::MultiImage::testPixel(const int2 &screen_pos) const {
		if(!rect.isInside(screen_pos))
			return false;
		for(int l = 0; l < layer_count; l++)
			if(images[l].testPixel(screen_pos - points[l] - rect.min))
				return true;
		return false;
	}


	void Sprite::load(Stream &sr) {
		sr.signature("SPRITE", 6);
		sr >> m_sequences >> m_frames >> m_palettes >> m_images;
		sr.unpack(m_offset, m_bbox);
	}

	void Sprite::save(Stream &sr) const {
		sr.signature("SPRITE", 6);
		sr << m_sequences << m_frames << m_palettes << m_images;
		sr.pack(m_offset, m_bbox);
	}
	

	void Sprite::clear() {
		m_sequences.clear();
		m_frames.clear();
		m_palettes.clear();
		m_images.clear();

		m_offset = int2(0, 0);
		m_bbox = int3(0, 0, 0);
	}

	bool Sprite::isSequenceLooped(int seq_id) const {
		const Sequence &seq = m_sequences[seq_id];
		for(int n = 0; n < seq.frame_count; n++)
			if(m_frames[seq.first_frame + n].id == ev_repeat_all)
				return true;
		return false;
	}

	int Sprite::imageIndex(int seq_id, int frame_id, int dir_id) const {
		DASSERT(seq_id >= 0 && seq_id < (int)m_sequences.size());

		const Sequence &seq = m_sequences[seq_id];
		DASSERT(frame_id >= 0 && frame_id < seq.frame_count);
		DASSERT(dir_id >= 0 && dir_id < seq.dir_count);

		const Frame &frame = m_frames[seq.first_frame + frame_id];
		DASSERT(frame.id >= 0);

		int out = frame.id + dir_id;
		DASSERT(out < (int)m_images.size());
		return out;
	}

	PTexture Sprite::getFrame(int seq_id, int frame_id, int dir_id, FRect &tex_rect) const {
		const MultiPalette &palette = m_palettes[m_sequences[seq_id].palette_id];
		return m_images[imageIndex(seq_id, frame_id, dir_id)].toTexture(palette, tex_rect);
	}

	IRect Sprite::getRect(int seq_id, int frame_id, int dir_id) const {
		return m_images[imageIndex(seq_id, frame_id, dir_id)].rect - m_offset;
	}

	IRect Sprite::getMaxRect() const {
		if(m_images.empty())
			return IRect::empty();

		IRect out = m_images[0].rect;
		for(int n = 1; n < (int)m_images.size(); n++)
			out = sum(out, m_images[n].rect);
		return out - m_offset;
	}
		
	bool Sprite::testPixel(const int2 &screen_pos, int seq_id, int frame_id, int dir_id) const {
		const MultiImage &image = m_images[imageIndex(seq_id, frame_id, dir_id)];
		return image.testPixel(screen_pos + m_offset);
	}

	int Sprite::findSequence(const char *name) const {
		for(int n = 0; n < (int)m_sequences.size(); n++)
			if(m_sequences[n].name == name)
				return n;
		return -1;
	}
		
	int Sprite::findDir(int seq_id, float radians) const {
		DASSERT(seq_id >= 0 && seq_id < size());
//		DASSERT(radians >= 0.0f && radians < constant::pi * 2.0f);

		//TODO: wtf???
		float2 vec = angleToVector(radians);
		radians = vectorToAngle(-vec);
		
		int dir_count = dirCount(seq_id);
		float dir = radians * float(dir_count) * (0.5f / constant::pi) + 0.5f;
		return (int(dir) + dir_count) % dir_count;
	}

	int Sprite::memorySize() const {
		size_t bytes = sizeof(Sprite) + sizeof(Frame) * m_frames.size();

		for(int n = 0; n < (int)m_sequences.size(); n++) {
			const Sequence &seq = m_sequences[n];
			bytes += sizeof(seq) + seq.name.size() + 1;
		}
		for(int i = 0; i < (int)m_palettes.size(); i++)
			bytes += sizeof(MultiPalette) + m_palettes[i].colors.size() * sizeof(Color);
		for(int n = 0; n < (int)m_images.size(); n++)
			bytes += m_images[n].memorySize();

		return (int)bytes;
	}

	void Sprite::printInfo() const {
		int img_bytes = 0, bytes = memorySize();
		for(int n = 0; n < (int)m_images.size(); n++)
			img_bytes += m_images[n].memorySize();
		int pal_bytes = (int)(m_palettes.size() * sizeof(MultiPalette));
		for(int p = 0; p < (int)m_palettes.size(); p++)
			pal_bytes += (int)m_palettes[p].colors.size() * sizeof(Color);

		printf("%30s: %6d KB (images: %6d KB, palettes: %4dKB)\n",
			resourceName(), bytes/1024, img_bytes/1024, pal_bytes/1024);
	}

	void Sprite::printSequencesInfo() const {
		for(int s = 0; s < (int)m_sequences.size(); s++) {
			const Sprite::Sequence &seq = m_sequences[s];
			printf("Sequence %d: %s (%d frames)\n", s, seq.name.c_str(), (int)seq.frame_count);
		}
	}

	void Sprite::printSequenceInfo(int seq_id) const {
		DASSERT(seq_id >= 0 && seq_id < (int)m_sequences.size());
		const Sequence &seq = m_sequences[seq_id];

		printf("Sequence %d (%s)\n", seq_id, seq.name.c_str());
		for(int n = 0; n < (int)seq.frame_count; n++)
			printf("frame #%d: %d\n", n, (int)m_frames[seq.first_frame + n].id);
		printf("\n");
	}

	ResourceMgr<Sprite> Sprite::mgr("data/sprites/", ".sprite");

}
