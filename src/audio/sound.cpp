/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "audio/sound.h"

namespace audio
{

	Sound::Sound() :m_frequency(1), m_bits(8), m_is_stereo(false) { }
		
	void Sound::save(Stream &sr) const {
		u32 chunk_size[2] = {(u32)m_data.size() + 36, 16};

		// Chunk 0
		sr.signature("RIFF", 4);
		sr << chunk_size[0];
		sr.signature("WAVE", 4);

		// Chunk 1
		sr.signature("fmt ", 4);
		sr << chunk_size[1];

		int num_channels = m_is_stereo? 2 : 1;
		int byte_rate = m_frequency * num_channels * (m_bits / 8), block_align = num_channels * (m_bits / 8);
		sr.pack(u16(1), u16(num_channels), u32(m_frequency), u32(byte_rate), u16(block_align), u16(m_bits));

		// Chunk 2
		sr.signature("data", 4);
		sr << u32(m_data.size());
		sr.saveData(m_data.data(), m_data.size());
	}

	void Sound::load(Stream &sr) {
		u32 chunk_size[2], size, frequency, byteRate;
		u16 format, channels, block_align, bits;

		// Chunk 0
		sr.signature("RIFF", 4);
		sr >> chunk_size[0];
		sr.signature("WAVE", 4);

		// Chunk 1
		sr.signature("fmt ", 4);
		sr.unpack(chunk_size[1], format, channels, frequency, byteRate, block_align, bits);
		ASSERT(block_align == channels * (bits / 8));

		if(format != 1)
			THROW("Unknown format (only PCM is supported)");

		sr.seek(sr.pos() + chunk_size[1] - 16);

		// Chunk 2
		sr.signature("data", 4);
		sr >> size;

		ASSERT(size >= 0);
		if(channels > 2 || (bits != 8 && bits != 16))
			THROW("Unknown format (bits: %d channels: %d)", bits, channels);

		m_data.resize(size);
		sr.loadData(m_data.data(), size);
		m_is_stereo = channels > 1;
		m_bits = bits;
		m_frequency = frequency;
	}
	
	double Sound::lengthInSeconds() const {
		return double(size()) / double(m_frequency * (m_is_stereo? 2 : 1) * (m_bits / 8));
	}

	void Sound::setData(const char *data, int size, int frequency, int bits, bool is_stereo) {
		DASSERT(size >= 0);
		DASSERT(bits == 8 || bits == 16);

		m_data.resize(size);
		memcpy(m_data.data(), data, size);
		m_frequency = frequency;
		m_bits = bits;
		m_is_stereo = is_stereo;
	}

}
