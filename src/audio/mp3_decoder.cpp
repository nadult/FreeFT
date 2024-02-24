// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "audio/mp3_decoder.h"
// TODO: enable this
#ifndef FWK_PLATFORM_WINDOWS
#include <mpg123.h>
#endif

#include <fwk/audio/sound.h>
#include <fwk/io/file_stream.h>

namespace audio {

#ifdef FWK_PLATFORM_WINDOWS
	// TODO: use browser mp3 decoder? But how?

	MP3Decoder::MP3Decoder(const string &file_name)
		: m_is_finished(true), m_sample_rate(44100), m_num_channels(1), m_need_data(false) {}
	MP3Decoder::~MP3Decoder() {}

	bool MP3Decoder::decode(Sound &out, int max_size) {
		out = {};
		return true;
	}
#else
	static int s_num_decoders = 0;

	//TODO: silence errors
	MP3Decoder::MP3Decoder(const string &file_name) {
		m_stream.emplace(move(fileLoader(file_name).get())); // TODO: pass it properly

		if(s_num_decoders++ == 0)
			mpg123_init();

		int ret = 0;
		auto handle = mpg123_new(0, &ret);
		if(!handle)
			FATAL("Error while creating mpg123 decoder");
		m_handle = handle;

		mpg123_open_feed(handle);
		//TODO: disable output
		mpg123_param(handle, MPG123_VERBOSE, 0.0, 0);

		m_is_finished = false;
		m_need_data = true;

		do {
			size_t size = 0;
			if(m_need_data) {
				PodVector<u8> input(min(4096, (int)(m_stream->size() - m_stream->pos())));
				m_stream->loadData(input);
				ret = mpg123_decode(handle, input.data(), input.size(), 0, 0, &size);
			} else {
				ret = mpg123_decode(handle, 0, 0, 0, 0, &size);
			}

			m_need_data = ret == MPG123_NEED_MORE;
		} while(ret != MPG123_ERR && ret != MPG123_NEW_FORMAT);

		if(ret == MPG123_ERR)
			FATAL("Error while opening mp3 file: %s\n", file_name.c_str());

		long rate;
		int enc;
		mpg123_getformat(handle, &rate, &m_num_channels, &enc);
		m_sample_rate = rate;

		if(enc != MPG123_ENC_SIGNED_16 || m_num_channels > 2)
			FATAL("Unsupported MP3 format (only 16-bit, with no more then 2 channels)");
	}

	MP3Decoder::~MP3Decoder() {
		if(m_handle)
			mpg123_delete((mpg123_handle *)m_handle);
		if(--s_num_decoders == 0)
			mpg123_exit();
	}

	bool MP3Decoder::decode(Sound &out, int max_size) {
		if(m_is_finished)
			return true;

		PodVector<char> temp(max_size);
		int out_pos = 0;
		auto handle = (mpg123_handle *)m_handle;

		int ret = 0;
		do {
			size_t size = 0;

			if(m_need_data) {
				int bytes_left = m_stream->size() - m_stream->pos();
				if(!bytes_left) {
					ret = MPG123_DONE;
					break;
				}
				PodVector<char> input(min(4096, bytes_left));

				m_stream->loadData(input);
				ret = mpg123_decode(handle, (u8 *)input.data(), input.size(),
									(u8 *)temp.data() + out_pos, max_size - out_pos, &size);
			} else {
				ret = mpg123_decode(handle, 0, 0, (u8 *)temp.data() + out_pos, max_size - out_pos,
									&size);
			}

			m_need_data = ret == MPG123_NEED_MORE;
			out_pos += size;
		} while(ret != MPG123_ERR && ret != MPG123_DONE && out_pos < max_size);

		m_is_finished = ret == MPG123_DONE || ret == MPG123_ERR;

		// TODO: this is bad
		temp.resize(out_pos);
		vector<char> out_data;
		temp.unsafeSwap(out_data);

		out = {move(out_data), {m_sample_rate, 16, m_num_channels == 2}};

		return m_is_finished;
	}
#endif

	int MP3Decoder::bytesPerSecond() const { return m_sample_rate * m_num_channels * 2; }

}
