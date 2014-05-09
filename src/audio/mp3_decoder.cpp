/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "audio/mp3_decoder.h"
#include "audio/sound.h"
#include <mpg123.h>

namespace audio {

	static int s_num_decoders = 0;

	//TODO: silence errors

	MP3Decoder::MP3Decoder(const string &file_name) :m_handle(nullptr) {
		m_stream.reset(new Loader(file_name));

		if(s_num_decoders++ == 0)
			mpg123_init();

		int ret = 0;
		m_handle = mpg123_new(0, &ret);
		if(!m_handle)
			THROW("Error while creating mpg123 decoder");

		mpg123_open_feed(m_handle);
		//TODO: disable output
		mpg123_param(m_handle, MPG123_VERBOSE, 0.0, 0);

		m_is_finished = false;
		m_need_data = true;

		do {
			size_t size = 0;
			if(m_need_data) {
				PodArray<u8> input(min(4096, (int)(m_stream->size() - m_stream->pos())));
				m_stream->loadData(input.data(), input.size());
				ret = mpg123_decode(m_handle, input.data(), input.size(), 0, 0, &size);
			}
			else {
				ret = mpg123_decode(m_handle, 0, 0, 0, 0, &size);
			}

			m_need_data = ret == MPG123_NEED_MORE;
		} while(ret != MPG123_ERR && ret != MPG123_NEW_FORMAT);

		if(ret == MPG123_ERR)
			THROW("Error while opening mp3 file: %s\n", file_name.c_str());

		long rate;
		int enc;
		mpg123_getformat(m_handle, &rate, &m_num_channels, &enc);
		m_sample_rate = rate;

		if(enc != MPG123_ENC_SIGNED_16 || m_num_channels > 2)
			THROW("Unsupported MP3 format (only 16-bit, with no more then 2 channels)");
	}

	MP3Decoder::~MP3Decoder() {
		if(m_handle)
			mpg123_delete(m_handle);
		if(--s_num_decoders == 0)
			mpg123_exit();
	}
		
	int MP3Decoder::bytesPerSecond() const {
		return m_sample_rate * m_num_channels * 2;
	}

	bool MP3Decoder::decode(Sound &out, int max_size) {
		if(m_is_finished)
			return true;

		PodArray<u8> temp(max_size);
		int out_pos = 0;

		int ret = 0;
		do {
			size_t size = 0;

			if(m_need_data) {
				int bytes_left = m_stream->size() - m_stream->pos();
				if(!bytes_left) {
					ret = MPG123_DONE;
					break;
				}
				PodArray<u8> input(min(4096, bytes_left));

				m_stream->loadData(input.data(), input.size());
				ret = mpg123_decode(m_handle, input.data(), input.size(), temp.data() + out_pos, max_size - out_pos, &size);
			}
			else {
				ret = mpg123_decode(m_handle, 0, 0, temp.data() + out_pos, max_size - out_pos, &size);
			}

			m_need_data = ret == MPG123_NEED_MORE;
			out_pos += size;
		} while(ret != MPG123_ERR && ret != MPG123_DONE && out_pos < max_size);

		m_is_finished = ret == MPG123_DONE || ret == MPG123_ERR;
		out.setData((const char*)temp.data(), out_pos, m_sample_rate, 16, m_num_channels == 2);

		return m_is_finished;
	}

}
