/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#ifndef AUDIO_MP3_DECODER_H
#define AUDIO_MP3_DECODER_H

#include "base.h"
#include "audio/sound.h"
#include <mpg123.h>

namespace audio {

	class Sound;

	class MP3Decoder {
	public:
		MP3Decoder(const string &file_name);
		~MP3Decoder();

		MP3Decoder(const MP3Decoder&) = delete;
		void operator=(const MP3Decoder&) = delete;

		bool decode(Sound &out, int max_size);
		bool isFinished() const { return m_is_finished; }

		int bytesPerSecond() const;
		
		//TODO: preloading data? (to avoid reading from disk while playing)
		//alternative: move audio device to separate thread
	private:
		std::unique_ptr<Stream> m_stream;
		mpg123_handle *m_handle;
		int m_sample_rate, m_num_channels;
		bool m_is_finished, m_need_data;
	};

}

#endif
