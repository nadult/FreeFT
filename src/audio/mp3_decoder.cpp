/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include "audio/mp3_decoder.h"
#include "audio/sound.h"
#include <mpg123.h>

namespace audio {

	static int s_num_decoders = 0;

	MP3Decoder::MP3Decoder(const string &file_name) :m_sample_rate(0), m_num_channels(0), m_handle(nullptr) {
		m_stream.reset(new Loader(file_name));
		printf("stream: %d\n", (int)m_stream->size());

		if(s_num_decoders++ == 0)
			mpg123_init();

		int ret = 0;
		m_handle = mpg123_new(0, &ret);
		if(!m_handle)
			THROW("Error while creating mpg123 decoder");

		mpg123_open_feed(m_handle);
		m_is_finished = false;
	}

	MP3Decoder::~MP3Decoder() {
		if(m_handle)
			mpg123_delete(m_handle);
		if(--s_num_decoders == 0)
			mpg123_exit();
	}

	bool MP3Decoder::decode(Sound &out, int input_size) {
		if(m_is_finished)
			return true;

		input_size = min(input_size, (int)(m_stream->size() - m_stream->pos()));

		PodArray<u8> input(input_size);
		vector<u8> output(input_size * 4);
		int out_size = 0;

		m_stream->loadData(input.data(), input.size());

		int ret = 0;
		do {
			size_t size = 0;
			ret = mpg123_decode(m_handle, input.isEmpty()? 0 : input.data(), input.size(), output.data() + out_size, output.size() - out_size, &size);
			input.clear();

			if(ret == MPG123_NEW_FORMAT) {
				long rate;
				int channels, enc;
				mpg123_getformat(m_handle, &rate, &channels, &enc);
				if((m_sample_rate != rate && m_sample_rate != 0) || (m_num_channels != channels && m_num_channels != 0) || enc != MPG123_ENC_SIGNED_16) {
					m_is_finished = true;
					m_stream.reset(nullptr);
					return true;
				}

				m_sample_rate = rate;
				m_num_channels = channels;
			}

			out_size += size;
			if(ret != MPG123_NEED_MORE)
				output.resize(out_size + input_size * 4);
		} while(ret != MPG123_ERR && ret != MPG123_NEED_MORE);

		m_is_finished = ret != MPG123_NEED_MORE;

		out.setData((const char*)output.data(), out_size, m_sample_rate, 16, m_num_channels == 2);

		return m_is_finished;
	}

}
