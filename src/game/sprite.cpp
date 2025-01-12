// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "game/sprite.h"

#include <fwk/gfx/image.h>
#include <fwk/io/file_stream.h>
#include <fwk/math/rotation.h>

namespace game {
static const char *s_event_names[6] = {
	"step_left", "step_right", "hit", "fire", "sound", "pickup",
};

const char *Sprite::eventIdToString(EventId id) {
	return id >= ev_pickup && id <= ev_step_left ? s_event_names[-40 - id] : nullptr;
}

int Sprite::MultiPalette::size(int layer) const {
	DASSERT(layer >= 0 && layer < layer_count);
	int next = layer == layer_count - 1 ? (int)colors.size() : offset[layer + 1];
	return next - offset[layer];
}

const Color *Sprite::MultiPalette::access(int layer) const {
	DASSERT(layer >= 0 && layer < layer_count);
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

Sprite::Sprite() : m_bbox(0, 0, 0), m_offset(0, 0), m_is_partial(false), m_index(-1) {}

Ex<Sprite::Sequence> Sprite::Sequence::load(FileStream &sr) {
	Sequence out;
	out.name = EX_PASS(loadString(sr));
	sr.unpack(out.frame_count, out.dir_count, out.first_frame, out.palette_id, out.overlay_id);
	return out;
}

void Sprite::Sequence::save(FileStream &sr) const {
	saveString(sr, name);
	sr.pack(frame_count, dir_count, first_frame, palette_id, overlay_id);
}

Ex<Sprite::MultiPalette> Sprite::MultiPalette::load(FileStream &sr) {
	MultiPalette out;
	u32 size = 0;
	sr >> size;
	out.colors.resize(size);
	sr.loadData(out.colors);
	sr.loadData(out.offset);
	return out;
}

void Sprite::MultiPalette::save(FileStream &sr) const {
	sr << u32(colors.size());
	sr.saveData(colors);
	sr.saveData(offset);
}

Ex<void> Sprite::MultiImage::load(FileStream &sr) {
	for(auto &image : images)
		image = EX_PASS(PackedTexture::load(sr));
	sr.loadData(points);
	sr >> rect;
	return {};
}

void Sprite::MultiImage::save(FileStream &sr) const {
	for(auto &image : images)
		image.save(sr);
	sr.saveData(points);
	sr << rect;
}

Sprite::MultiImage::MultiImage() : prev_palette(nullptr) {}

Sprite::MultiImage::MultiImage(const MultiImage &rhs) : prev_palette(nullptr) { *this = rhs; }

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

void Sprite::MultiImage::cacheUpload(Image &image) const {
	DASSERT(prev_palette);

	image.resize(rect.size());
	if(!image.empty()) {
		image.fill(IColor(0, 0, 0, 0));
		for(int l = 0; l < Sprite::layer_count; l++)
			images[l].blit(image, points[l], prev_palette->access(l), prev_palette->size(l));
	}
}

PVImageView Sprite::MultiImage::toTexture(const MultiPalette &palette, FRect &tex_rect,
										  bool put_in_atlas) const {
	if(cacheId() == -1) {
		bindToCache();
		prev_palette = &palette;
	}

	return accessTexture(tex_rect, put_in_atlas);
}

bool Sprite::MultiImage::testPixel(const int2 &screen_pos) const {
	if(!rect.containsCell(screen_pos))
		return false;
	for(int l = 0; l < layer_count; l++)
		if(images[l].testPixel(screen_pos - points[l] - rect.min()))
			return true;
	return false;
}

Ex<void> Sprite::load(FileStream &sr, bool full_load) {
	EXPECT(sr.loadSignature("SPRITE"));

	sr.unpack(m_offset, m_bbox);
	u32 size;
	sr >> size;
	m_sequences.reserve(size);
	for(int n : intRange(size))
		m_sequences.emplace_back(EX_PASS(Sequence::load(sr)));
	sr >> size;
	m_frames.resize(size);
	sr.loadData(m_frames);
	sr >> m_max_rect;

	m_is_partial = !full_load;
	if(full_load) {
		sr >> size;
		m_palettes.reserve(size);
		for(int n : intRange(size))
			m_palettes.emplace_back(EX_PASS(MultiPalette::load(sr)));

		sr >> size;
		m_images.resize(size);
		for(auto &image : m_images)
			EXPECT(image.load(sr));
	} else {
		m_palettes.clear();
		m_images.clear();
	}
	return {};
}

void Sprite::save(FileStream &sr) const {
	sr.saveSignature("SPRITE");
	sr.pack(m_offset, m_bbox);
	sr << u32(m_sequences.size());
	for(auto &seq : m_sequences)
		seq.save(sr);
	sr << m_frames.size();
	sr.saveData(m_frames);
	sr << m_max_rect;
	sr << u32(m_palettes.size());
	for(auto &pal : m_palettes)
		pal.save(sr);
	sr << u32(m_images.size());
	for(auto &image : m_images)
		image.save(sr);
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

PVImageView Sprite::getFrame(int seq_id, int frame_id, int dir_id, FRect &tex_rect,
							 bool put_in_atlas) const {
	DASSERT(!isPartial());

	const MultiPalette &palette = m_palettes[m_sequences[seq_id].palette_id];
	return m_images[imageIndex(seq_id, frame_id, dir_id)].toTexture(palette, tex_rect,
																	put_in_atlas);
}

IRect Sprite::getRect(int seq_id, int frame_id, int dir_id) const {
	DASSERT(!isPartial());

	return m_images[imageIndex(seq_id, frame_id, dir_id)].rect - m_offset;
}

void Sprite::updateMaxRect() {
	if(m_images.empty())
		m_max_rect = IRect();

	IRect out = m_images[0].rect;
	for(int n = 1; n < (int)m_images.size(); n++)
		out = enclose(out, m_images[n].rect);
	m_max_rect = out - m_offset;
}

bool Sprite::testPixel(const int2 &screen_pos, int seq_id, int frame_id, int dir_id) const {
	const MultiImage &image = m_images[imageIndex(seq_id, frame_id, dir_id)];
	return image.testPixel(screen_pos + m_offset);
}

int Sprite::findSequence(Str name) const {
	for(int n = 0; n < (int)m_sequences.size(); n++)
		if(name.compareIgnoreCase(m_sequences[n].name) == 0)
			return n;
	return -1;
}

int Sprite::findDir(int seq_id, float radians) const {
	DASSERT(seq_id >= 0 && seq_id < size());
	//		DASSERT(radians >= 0.0f && radians < pi * 2.0f);

	//TODO: wtf???
	float2 vec = angleToVector(radians);
	radians = vectorToAngle(-vec);

	int dir_count = dirCount(seq_id);
	float dir = radians * float(dir_count) * (0.5f / pi) + 0.5f;
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

	printf("%30s: %6d KB (images: %6d KB, palettes: %4dKB)\n", m_resource_name.c_str(),
		   bytes / 1024, img_bytes / 1024, pal_bytes / 1024);
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

}
